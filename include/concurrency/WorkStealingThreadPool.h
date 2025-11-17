#pragma once

#include <atomic>
#include <thread>
#include <vector>
#include <functional>
#include <memory>
#include <chrono>
#include "concurrency/LockFreeRingBuffer.h"
#include <deque>
#include <mutex>

namespace NeonGlyph {
namespace Concurrency {

class WorkStealingThreadPool {
public:
    explicit WorkStealingThreadPool(size_t workers = std::thread::hardware_concurrency(), size_t queueCapacity = 1 << 16, bool pinThreads = false)
        : m_shutdown(false), m_pin(pinThreads) {
        if (workers == 0) workers = 1;
        m_deques.resize(workers);
        m_mutexes.resize(workers);
        m_queueDepths.resize(workers);
        for (size_t i = 0; i < workers; ++i) {
            m_workers.emplace_back([this, i]() { this->WorkerLoop(i); });
        }
    }

    ~WorkStealingThreadPool() {
        shutdown();
    }

    bool submit(const std::function<void()>& task) {
        size_t idx = m_rr.fetch_add(1, std::memory_order_relaxed) % m_deques.size();
        {
            std::lock_guard<std::mutex> lock(m_mutexes[idx]);
            m_deques[idx].push_front(task);
            m_queueDepths[idx].fetch_add(1, std::memory_order_relaxed);
        }
        return true;
    }

    void shutdown() {
        bool expected = false;
        if (m_shutdown.compare_exchange_strong(expected, true)) {
            for (auto& t : m_workers) {
                if (t.joinable()) t.join();
            }
            m_workers.clear();
        }
    }

private:
    void WorkerLoop(size_t self) {
        if (m_pin) {
#ifdef _WIN32
            DWORD_PTR mask = 1ull << (self % 64);
            SetThreadAffinityMask(GetCurrentThread(), mask);
#endif
        }
        while (!m_shutdown.load(std::memory_order_relaxed)) {
            std::function<void()> task;
            {
                std::lock_guard<std::mutex> lock(m_mutexes[self]);
                if (!m_deques[self].empty()) {
                    task = m_deques[self].front();
                    m_deques[self].pop_front();
                    m_queueDepths[self].fetch_sub(1, std::memory_order_relaxed);
                }
            }
            if (!task) {
                size_t n = m_deques.size();
                for (size_t i = 0; i < n; ++i) {
                    size_t victim = (self + i + 1) % n;
                    std::lock_guard<std::mutex> lock(m_mutexes[victim]);
                    if (!m_deques[victim].empty()) {
                        task = m_deques[victim].back();
                        m_deques[victim].pop_back();
                        m_queueDepths[victim].fetch_sub(1, std::memory_order_relaxed);
                        break;
                    }
                }
            }
            if (task) {
                task();
            } else {
                std::this_thread::sleep_for(std::chrono::microseconds(50));
            }
        }
    }

    std::vector<std::deque<std::function<void()>>> m_deques;
    std::vector<std::mutex> m_mutexes;
    std::vector<std::atomic<int>> m_queueDepths;
    std::vector<std::thread> m_workers;
    std::atomic<size_t> m_rr{0};
    std::atomic<bool> m_shutdown;
    bool m_pin;
public:
    std::vector<int> queueDepths() const {
        std::vector<int> d(m_queueDepths.size());
        for (size_t i = 0; i < m_queueDepths.size(); ++i) d[i] = m_queueDepths[i].load(std::memory_order_relaxed);
        return d;
    }
};

} // namespace Concurrency
} // namespace NeonGlyph