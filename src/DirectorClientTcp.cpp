#include "DirectorClient.h"
#include "DirectorTypes.h"
#include "Logger.h"
#include <nlohmann/json.hpp>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include "concurrency/LockFreeRingBuffer.h"
#include "chaos/IocpMetrics.h"
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <chrono>
#include <sstream>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

namespace NeonGlyph {

using json = nlohmann::json;

class TcpDirectorClient : public DirectorClient {
public:
    TcpDirectorClient(const std::string& host, int port);
    ~TcpDirectorClient() override;

    void Pump() override;
    bool TryGetLatestDirective(DirectorDirective& outDir) override;

private:
    void WorkerThread();
    bool Connect();
    void Disconnect();
    bool SendJson(const json& j);
    bool ReceiveJson(json& j);
    void ProcessIncomingMessages();
    void IocpThread();
    void SenderLoop();
    
    std::string m_host;
    int m_port;
    SOCKET m_socket = INVALID_SOCKET;
    std::thread m_workerThread;
    std::thread m_iocpThread;
    std::thread m_senderThread;
    std::atomic<bool> m_shouldStop{false};
    std::atomic<bool> m_connected{false};
    std::atomic<bool> m_useIocp{false};
    HANDLE m_iocp{NULL};
    WSABUF m_recvBuf{};
    OVERLAPPED m_recvOv{};
    std::string m_recvAccum;
    struct SendCtx { OVERLAPPED ov; WSABUF buf; std::string data; };
    NeonGlyph::Concurrency::LockFreeRingBuffer<std::string> m_sendQueue{4096};
    std::atomic<int> m_sendQueueDepth{0};
    std::atomic<int> m_outstandingSends{0};
    int m_maxOutstandingSends{64};
    std::atomic<int> m_sendRateBps{512000};
    std::atomic<int> m_bucketSize{1048576};
    std::atomic<int> m_tokens{0};
    std::chrono::steady_clock::time_point m_lastTokenRefill;
    
    // Thread-safe directive queue
    std::mutex m_directiveMutex;
    DirectorDirective m_latestDirective;
    std::atomic<bool> m_hasNewDirective{false};
    
    // State snapshot queue (thread-safe)
    std::mutex m_snapshotMutex;
    std::queue<DirectorStateSnapshot> m_snapshotQueue;
    
    // Retry logic
    std::chrono::steady_clock::time_point m_lastConnectAttempt;
    int m_retryDelayMs = 1000;
    int m_maxRetryDelayMs = 30000;
};

TcpDirectorClient::TcpDirectorClient(const std::string& host, int port) 
    : m_host(host), m_port(port) {
    
    // Initialize Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
    }
    
    m_lastConnectAttempt = std::chrono::steady_clock::now() - std::chrono::seconds(60);
    m_lastTokenRefill = std::chrono::steady_clock::now();
    m_tokens.store(m_bucketSize.load());
    
    // Start worker thread
    m_workerThread = std::thread(&TcpDirectorClient::WorkerThread, this);
}

TcpDirectorClient::~TcpDirectorClient() {
    m_shouldStop = true;
    
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
    if (m_iocpThread.joinable()) {
        PostQueuedCompletionStatus(m_iocp, 0, 0, NULL);
        m_iocpThread.join();
    }
    if (m_senderThread.joinable()) {
        m_senderThread.join();
    }
    
    Disconnect();
    WSACleanup();
}

void TcpDirectorClient::WorkerThread() {
    std::cout << "DirectorClient worker thread started" << std::endl;
    
    while (!m_shouldStop) {
        // Try to connect if not connected
        if (!m_connected) {
            auto now = std::chrono::steady_clock::now();
            auto timeSinceLastAttempt = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastConnectAttempt);
            
            if (timeSinceLastAttempt.count() >= m_retryDelayMs) {
                if (Connect()) {
                    m_retryDelayMs = 1000; // Reset retry delay on successful connection
                    std::cout << "Connected to Director Daemon at " << m_host << ":" << m_port << std::endl;
                } else {
                    // Exponential backoff
                    m_retryDelayMs = std::min(m_retryDelayMs * 2, m_maxRetryDelayMs);
                    std::cout << "Failed to connect to Director Daemon, retrying in " << m_retryDelayMs << " ms" << std::endl;
                }
                m_lastConnectAttempt = now;
            }
        }
        
        if (m_connected && !m_useIocp) {
            ProcessIncomingMessages();
        }
        {
            auto now = std::chrono::steady_clock::now();
            auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastTokenRefill).count();
            if (elapsedMs > 0) {
                long add = static_cast<long>((static_cast<long long>(m_sendRateBps.load()) * elapsedMs) / 1000);
                int current = m_tokens.load();
                long next = std::min<long>(current + add, m_bucketSize.load());
                m_tokens.store(static_cast<int>(next));
                m_lastTokenRefill = now;
            }
        }
        
        // Small delay to prevent busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }
    
    std::cout << "DirectorClient worker thread stopped" << std::endl;
}

bool TcpDirectorClient::Connect() {
    if (m_socket != INVALID_SOCKET) {
        Disconnect();
    }
    
    // Create socket
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        return false;
    }
    
    // Set socket to non-blocking mode
    u_long mode = 1;
    if (ioctlsocket(m_socket, FIONBIO, &mode) != 0) {
        std::cerr << "Failed to set non-blocking mode: " << WSAGetLastError() << std::endl;
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }
    
    // Resolve address
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(static_cast<u_short>(m_port));
    
    if (inet_pton(AF_INET, m_host.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address: " << m_host << std::endl;
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }
    
    // Connect (non-blocking, will return immediately)
    int result = connect(m_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK) {
            std::cerr << "Connect failed: " << error << std::endl;
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            return false;
        }
        // Connection in progress, will complete asynchronously
    }
    
    // Check if connection completed
    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(m_socket, &writeSet);
    
    struct timeval timeout;
    timeout.tv_sec = 5; // 5 second timeout
    timeout.tv_usec = 0;
    
    result = select(0, nullptr, &writeSet, nullptr, &timeout);
    if (result <= 0) {
        std::cerr << "Connection timeout or error" << std::endl;
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }
    
    // Check for connection errors
    int error;
    socklen_t len = sizeof(error);
    if (getsockopt(m_socket, SOL_SOCKET, SO_ERROR, (char*)&error, &len) < 0 || error != 0) {
        std::cerr << "Connection failed: " << error << std::endl;
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }
    
    m_connected = true;
    if (!m_iocp) {
        m_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    }
    CreateIoCompletionPort((HANDLE)m_socket, m_iocp, 0, 0);
    memset(&m_recvOv, 0, sizeof(m_recvOv));
    char* buf = new char[8192];
    m_recvBuf.buf = buf;
    m_recvBuf.len = 8192;
    DWORD flags = 0;
    DWORD bytes = 0;
    WSARecv(m_socket, &m_recvBuf, 1, &bytes, &flags, &m_recvOv, NULL);
    m_useIocp = true;
    if (!m_iocpThread.joinable()) {
        m_iocpThread = std::thread(&TcpDirectorClient::IocpThread, this);
    }
    if (!m_senderThread.joinable()) {
        m_senderThread = std::thread(&TcpDirectorClient::SenderLoop, this);
    }
    return true;
}

void TcpDirectorClient::Disconnect() {
    m_connected = false;
    
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
    if (m_iocp) {
        CloseHandle(m_iocp);
        m_iocp = NULL;
    }
    if (m_recvBuf.buf) {
        delete[] m_recvBuf.buf;
        m_recvBuf.buf = NULL;
        m_recvBuf.len = 0;
    }
}

void TcpDirectorClient::Pump() {
    // This method is called from the main thread to send state snapshots
    // and process any pending messages
    
    DirectorStateSnapshot snapshot;
    
    // Get snapshot from queue (if available)
    {
        std::lock_guard<std::mutex> lock(m_snapshotMutex);
        if (!m_snapshotQueue.empty()) {
            snapshot = m_snapshotQueue.front();
            m_snapshotQueue.pop();
        } else {
            return; // No snapshot to send
        }
    }
    
    // Send snapshot if connected
    if (m_connected && m_socket != INVALID_SOCKET) {
        json stateJson;
        stateJson["time"] = snapshot.time;
        stateJson["audio"]["rms"] = snapshot.audio.rms;
        stateJson["audio"]["bass"] = snapshot.audio.bass;
        stateJson["audio"]["mids"] = snapshot.audio.mids;
        stateJson["audio"]["highs"] = snapshot.audio.highs;
        stateJson["scene"]["id"] = snapshot.scene.id;
        stateJson["scene"]["intensity"] = snapshot.scene.intensity;
        stateJson["scene"]["palette"] = snapshot.scene.palette;
        
        if (!SendJson(stateJson)) {
            std::cout << "Failed to send state snapshot" << std::endl;
            Disconnect();
        }
    }
}

bool TcpDirectorClient::TryGetLatestDirective(DirectorDirective& outDir) {
    std::lock_guard<std::mutex> lock(m_directiveMutex);
    
    if (m_hasNewDirective) {
        outDir = m_latestDirective;
        m_hasNewDirective = false;
        return true;
    }
    
    return false;
}

bool TcpDirectorClient::SendJson(const json& j) {
    std::string message = j.dump() + "\n";
    if (m_useIocp) {
        if (m_sendQueue.enqueue(message)) { m_sendQueueDepth.fetch_add(1, std::memory_order_relaxed); return true; }
        return false;
    }
    int need = static_cast<int>(message.size());
    int have = m_tokens.load();
    if (have < need) {
        return false;
    }
    m_tokens.store(have - need);
    if (m_useIocp) {
        WSABUF sbuf;
        sbuf.buf = const_cast<char*>(message.data());
        sbuf.len = static_cast<ULONG>(message.size());
        OVERLAPPED sov{};
        DWORD bytes = 0;
        int r = WSASend(m_socket, &sbuf, 1, &bytes, 0, &sov, NULL);
        if (r == SOCKET_ERROR) {
            int e = WSAGetLastError();
            if (e != WSA_IO_PENDING) {
                return false;
            }
        }
        return true;
    } else {
        const char* data = message.c_str();
        size_t remaining = message.length();
        while (remaining > 0) {
            int sent = send(m_socket, data, static_cast<int>(remaining), 0);
            if (sent == SOCKET_ERROR) {
                int error = WSAGetLastError();
                if (error == WSAEWOULDBLOCK) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }
                return false;
            }
            data += sent;
            remaining -= sent;
        }
        return true;
    }
}

bool TcpDirectorClient::ReceiveJson(json& j) {
    std::string buffer;
    char temp[4096];
    
    while (true) {
        int received = recv(m_socket, temp, sizeof(temp) - 1, 0);
        if (received == SOCKET_ERROR) {
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK) {
                // No data available, try again later
                break;
            }
            std::cerr << "Receive failed: " << error << std::endl;
            return false;
        }
        
        if (received == 0) {
            // Connection closed
            std::cout << "Connection closed by server" << std::endl;
            return false;
        }
        
        temp[received] = '\0';
        buffer += temp;
        
        // Check if we have a complete JSON message (ends with newline)
        size_t newlinePos = buffer.find('\n');
        if (newlinePos != std::string::npos) {
            std::string jsonStr = buffer.substr(0, newlinePos);
            buffer = buffer.substr(newlinePos + 1);
            
            try {
                j = json::parse(jsonStr);
                return true;
            } catch (const std::exception& e) {
                std::cerr << "JSON parse error: " << e.what() << std::endl;
                continue;
            }
        }
    }
    
    return false;
}

void TcpDirectorClient::ProcessIncomingMessages() {
    json directiveJson;
    
    while (ReceiveJson(directiveJson)) {
        try {
            DirectorDirective directive;
            directive.sceneId = directiveJson.value("sceneId", "");
            directive.motionPreset = directiveJson.value("motionPreset", "");
            directive.paletteShift = directiveJson.value("paletteShift", "");
            directive.intensity = directiveJson.value("intensity", 1.0f);
            directive.mood = directiveJson.value("mood", "");
            
            if (directiveJson.contains("microEvents")) {
                for (const auto& event : directiveJson["microEvents"]) {
                    directive.microEvents.push_back(event.get<std::string>());
                }
            }
            
            // Update latest directive
            {
                std::lock_guard<std::mutex> lock(m_directiveMutex);
                m_latestDirective = directive;
                m_hasNewDirective = true;
            }
            
            std::cout << "Received directive: scene=" << directive.sceneId 
                     << ", mood=" << directive.mood 
                     << ", intensity=" << directive.intensity << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "Failed to parse directive: " << e.what() << std::endl;
        }
    }
}

void TcpDirectorClient::IocpThread() {
    while (!m_shouldStop) {
        DWORD bytes = 0; ULONG_PTR key = 0; LPOVERLAPPED ov = nullptr;
        BOOL ok = GetQueuedCompletionStatus(m_iocp, &bytes, &key, &ov, INFINITE);
        if (!ok || ov == NULL) {
            break;
        }
        if (bytes == 0) {
            m_connected = false;
            break;
        }
        if (ov == &m_recvOv) {
            m_recvAccum.append(m_recvBuf.buf, m_recvBuf.buf + bytes);
            NeonGlyph::Chaos::IocpMetrics::IncRecv();
        } else {
            SendCtx* ctx = reinterpret_cast<SendCtx*>(ov);
            m_outstandingSends.fetch_sub(1, std::memory_order_relaxed);
            NeonGlyph::Chaos::IocpMetrics::IncSend();
            delete ctx;
            continue;
        }
        size_t pos;
        while ((pos = m_recvAccum.find('\n')) != std::string::npos) {
            std::string jsonStr = m_recvAccum.substr(0, pos);
            m_recvAccum.erase(0, pos + 1);
            try {
                json directiveJson = json::parse(jsonStr);
                DirectorDirective directive;
                directive.sceneId = directiveJson.value("sceneId", "");
                directive.motionPreset = directiveJson.value("motionPreset", "");
                directive.paletteShift = directiveJson.value("paletteShift", "");
                directive.intensity = directiveJson.value("intensity", 1.0f);
                directive.mood = directiveJson.value("mood", "");
                if (directiveJson.contains("microEvents")) {
                    for (const auto& event : directiveJson["microEvents"]) {
                        directive.microEvents.push_back(event.get<std::string>());
                    }
                }
                {
                    std::lock_guard<std::mutex> lock(m_directiveMutex);
                    m_latestDirective = directive;
                    m_hasNewDirective = true;
                }
            } catch (...) {
            }
        }
        DWORD flags = 0; DWORD b = 0; memset(&m_recvOv, 0, sizeof(m_recvOv));
        WSARecv(m_socket, &m_recvBuf, 1, &b, &flags, &m_recvOv, NULL);
    }
}

void TcpDirectorClient::SenderLoop() {
    while (!m_shouldStop) {
        if (!m_connected || !m_useIocp) { std::this_thread::sleep_for(std::chrono::milliseconds(5)); continue; }
        while (m_outstandingSends.load(std::memory_order_relaxed) < m_maxOutstandingSends) {
            std::string msg;
            if (!m_sendQueue.dequeue(msg)) break;
            m_sendQueueDepth.fetch_sub(1, std::memory_order_relaxed);
            WSABUF sbuf; sbuf.buf = const_cast<char*>(msg.data()); sbuf.len = static_cast<ULONG>(msg.size());
            SendCtx* ctx = new SendCtx{}; ctx->buf = sbuf; ctx->data = std::move(msg);
            DWORD bytes = 0; m_outstandingSends.fetch_add(1, std::memory_order_relaxed);
            int r = WSASend(m_socket, &ctx->buf, 1, &bytes, 0, &ctx->ov, NULL);
            if (r == SOCKET_ERROR) {
                int e = WSAGetLastError();
                if (e != WSA_IO_PENDING) { m_outstandingSends.fetch_sub(1, std::memory_order_relaxed); delete ctx; }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// Factory function implementation
std::unique_ptr<DirectorClient> CreateDirectorClient(const std::string& host, int port) {
    return std::make_unique<TcpDirectorClient>(host, port);
}

} // namespace NeonGlyph