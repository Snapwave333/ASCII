#pragma once

#include <atomic>
#include <cstddef>
#include <type_traits>
#include <vector>
#include <stdexcept>
#include <new>

namespace NeonGlyph {
namespace Concurrency {

template <typename T>
class LockFreeRingBuffer {
public:
    explicit LockFreeRingBuffer(size_t capacity)
        : m_capacity(RoundUpToPowerOfTwo(capacity)),
          m_mask(m_capacity - 1),
          m_buffer(m_capacity),
          m_head(0),
          m_tail(0) {
        if (m_capacity == 0) throw std::invalid_argument("capacity must be > 0");
        for (size_t i = 0; i < m_capacity; ++i) {
            m_seq[i].store(i, std::memory_order_relaxed);
        }
    }

    bool enqueue(const T& value) {
        size_t pos;
        for (;;) {
            pos = m_tail.load(std::memory_order_relaxed);
            if (m_seq[pos & m_mask].load(std::memory_order_acquire) == pos) {
                if (m_tail.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                    m_buffer[pos & m_mask] = value;
                    m_seq[pos & m_mask].store(pos + 1, std::memory_order_release);
                    return true;
                }
            } else {
                // full
                return false;
            }
        }
    }

    bool dequeue(T& out) {
        size_t pos;
        for (;;) {
            pos = m_head.load(std::memory_order_relaxed);
            if (m_seq[pos & m_mask].load(std::memory_order_acquire) == pos + 1) {
                if (m_head.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                    out = m_buffer[pos & m_mask];
                    m_seq[pos & m_mask].store(pos + m_mask + 1, std::memory_order_release);
                    return true;
                }
            } else {
                // empty
                return false;
            }
        }
    }

    size_t capacity() const { return m_capacity; }

private:
    static size_t RoundUpToPowerOfTwo(size_t n) {
        if (n == 0) return 0;
        n--; n |= n >> 1; n |= n >> 2; n |= n >> 4; n |= n >> 8; n |= n >> 16;
#if SIZE_MAX > 0xFFFFFFFFu
        n |= n >> 32;
#endif
        return n + 1;
    }

    const size_t m_capacity;
    const size_t m_mask;
    std::vector<T> m_buffer;
    alignas(64) std::atomic<size_t> m_head;
    alignas(64) std::atomic<size_t> m_tail;
    alignas(64) std::vector<std::atomic<size_t>> m_seq = std::vector<std::atomic<size_t>>(m_capacity);
};

} // namespace Concurrency
} // namespace NeonGlyph