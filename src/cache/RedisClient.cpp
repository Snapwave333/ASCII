#include "cache/RedisClient.h"
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif
#include <sstream>

using namespace NeonGlyph::Cache;

static std::string respBulk(const std::string& s) {
    std::ostringstream o; o << "$" << s.size() << "\r\n" << s << "\r\n"; return o.str();
}
static std::string respArray(const std::vector<std::string>& parts) {
    std::ostringstream o; o << "*" << parts.size() << "\r\n"; for (auto& p : parts) o << respBulk(p); return o.str();
}

RedisConnection::RedisConnection() : m_sock(nullptr) {}
RedisConnection::~RedisConnection() { if (m_sock) { closesocket((SOCKET)m_sock); m_sock = nullptr; } }
bool RedisConnection::connect(const std::string& host, int port) {
#ifdef _WIN32
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return false;
    sockaddr_in addr{}; addr.sin_family = AF_INET; addr.sin_port = htons((u_short)port);
    inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
    if (::connect(s, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) { closesocket(s); return false; }
    m_sock = (void*)s; return true;
#else
    return false;
#endif
}
bool RedisConnection::send(const std::string& data) {
    if (!m_sock) return false;
    const char* p = data.data(); size_t r = data.size();
    while (r) { int n = ::send((SOCKET)m_sock, p, (int)r, 0); if (n <= 0) return false; p += n; r -= n; }
    return true;
}
std::optional<std::string> RedisConnection::read() {
    if (!m_sock) return std::nullopt;
    char buf[4096]; int n = ::recv((SOCKET)m_sock, buf, sizeof(buf), 0);
    if (n <= 0) return std::nullopt; return std::string(buf, buf + n);
}
std::optional<std::string> RedisConnection::command(const std::string& cmd) {
    if (!send(cmd)) return std::nullopt; return read();
}

RedisClientPool::RedisClientPool() {}
RedisClientPool::~RedisClientPool() { if (m_subThread.joinable()) m_subThread.join(); for (auto* c : m_pool) { delete c; } }
bool RedisClientPool::init(const std::string& endpoint, int poolSize) {
    std::string ep = endpoint; std::string host = "127.0.0.1"; int port = 6379;
    auto pos = ep.find(":"); if (pos != std::string::npos) { host = ep.substr(0, pos); port = std::stoi(ep.substr(pos + 1)); }
    std::lock_guard<std::mutex> lock(m_mutex);
    for (int i = 0; i < poolSize; ++i) { auto* c = new RedisConnection(); if (!c->connect(host, port)) { delete c; continue; } m_pool.push_back(c); }
    m_ready.store(!m_pool.empty()); return m_ready.load();
}
std::optional<std::string> RedisClientPool::get(const std::string& key) {
    if (!m_ready.load()) return std::nullopt; std::lock_guard<std::mutex> lock(m_mutex);
    if (m_pool.empty()) return std::nullopt; auto* c = m_pool[0]; auto cmd = respArray({"GET", key}); return c->command(cmd);
}
bool RedisClientPool::set(const std::string& key, const std::string& value) {
    if (!m_ready.load()) return false; std::lock_guard<std::mutex> lock(m_mutex);
    if (m_pool.empty()) return false; auto* c = m_pool[0]; auto cmd = respArray({"SET", key, value}); auto r = c->command(cmd); return r.has_value();
}
std::vector<std::optional<std::string>> RedisClientPool::mget(const std::vector<std::string>& keys) {
    std::vector<std::optional<std::string>> out; if (!m_ready.load()) return out; std::lock_guard<std::mutex> lock(m_mutex);
    if (m_pool.empty()) return out; auto* c = m_pool[0]; std::vector<std::string> parts{"MGET"}; parts.insert(parts.end(), keys.begin(), keys.end()); auto cmd = respArray(parts); auto r = c->command(cmd); out.push_back(r); return out;
}
bool RedisClientPool::mset(const std::vector<std::pair<std::string, std::string>>& kvs) {
    if (!m_ready.load()) return false; std::lock_guard<std::mutex> lock(m_mutex);
    if (m_pool.empty()) return false; auto* c = m_pool[0]; std::vector<std::string> parts{"MSET"}; for (auto& kv : kvs) { parts.push_back(kv.first); parts.push_back(kv.second); } auto cmd = respArray(parts); auto r = c->command(cmd); return r.has_value();
}
std::vector<std::optional<std::string>> RedisClientPool::pipeline(const std::vector<std::string>& cmds) {
    std::vector<std::optional<std::string>> out; if (!m_ready.load()) return out; std::lock_guard<std::mutex> lock(m_mutex);
    if (m_pool.empty()) return out; auto* c = m_pool[0]; std::string batch; for (auto& cmd : cmds) batch += cmd; if (!c->send(batch)) return out; auto r = c->read(); out.push_back(r); return out;
}
bool RedisClientPool::subscribe(const std::string& channel, const std::function<void(const std::string&)>& cb) {
    if (!m_ready.load()) return false; if (m_subThread.joinable()) return false;
    auto* c = new RedisConnection(); if (!c->connect("127.0.0.1", 6379)) { delete c; return false; }
    m_subThread = std::thread([c, channel, cb]() { auto cmd = respArray({"SUBSCRIBE", channel}); c->send(cmd); for (;;) { auto r = c->read(); if (!r.has_value()) break; cb(*r); } delete c; });
    return true;
}