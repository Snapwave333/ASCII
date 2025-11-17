#pragma once

#include <unordered_map>
#include <list>
#include <vector>
#include <mutex>
#include <memory>
#include <functional>
#include <optional>
#include <chrono>

namespace NeonGlyph {
namespace Cache {

template <typename Key, typename Value, typename Hash = std::hash<Key>>
class SegmentedLRUCache {
public:
    struct Entry {
        Key key;
        Value value;
        std::chrono::steady_clock::time_point expiresAt;
    };

    explicit SegmentedLRUCache(size_t capacity, size_t segments = 16, std::chrono::seconds ttl = std::chrono::seconds(60))
        : m_capacity(capacity), m_segments(segments), m_ttl(ttl) {
        if (m_segments == 0) m_segments = 1;
        m_shards.resize(m_segments);
        for (auto& s : m_shards) {
            s.capacity = capacity / m_segments + 1;
            s.mutex = std::make_unique<std::mutex>();
        }
    }

    void set(const Key& key, const Value& value) {
        auto& shard = getShard(key);
        std::lock_guard<std::mutex> lock(*shard.mutex);
        auto it = shard.map.find(key);
        if (it != shard.map.end()) {
            it->second->value = value;
            it->second->expiresAt = std::chrono::steady_clock::now() + m_ttl;
            shard.lru.splice(shard.lru.begin(), shard.lru, it->second);
        } else {
            Entry e{key, value, std::chrono::steady_clock::now() + m_ttl};
            shard.lru.push_front(e);
            shard.map[key] = shard.lru.begin();
            if (shard.map.size() > shard.capacity) {
                auto last = shard.lru.end(); --last;
                shard.map.erase(last->key);
                shard.lru.pop_back();
            }
        }
    }

    std::optional<Value> get(const Key& key) {
        auto& shard = getShard(key);
        std::lock_guard<std::mutex> lock(*shard.mutex);
        auto it = shard.map.find(key);
        if (it == shard.map.end()) { m_misses.fetch_add(1, std::memory_order_relaxed); return std::nullopt; }
        if (std::chrono::steady_clock::now() > it->second->expiresAt) {
            shard.lru.erase(it->second);
            shard.map.erase(it);
            m_misses.fetch_add(1, std::memory_order_relaxed);
            return std::nullopt;
        }
        shard.lru.splice(shard.lru.begin(), shard.lru, it->second);
        m_hits.fetch_add(1, std::memory_order_relaxed);
        return it->second->value;
    }

    void invalidate(const Key& key) {
        auto& shard = getShard(key);
        std::lock_guard<std::mutex> lock(*shard.mutex);
        auto it = shard.map.find(key);
        if (it != shard.map.end()) {
            shard.lru.erase(it->second);
            shard.map.erase(it);
        }
    }

    uint64_t hits() const { return m_hits.load(std::memory_order_relaxed); }
    uint64_t misses() const { return m_misses.load(std::memory_order_relaxed); }

private:
    struct Shard {
        size_t capacity{0};
        std::list<Entry> lru;
        std::unordered_map<Key, typename std::list<Entry>::iterator, Hash> map;
        std::unique_ptr<std::mutex> mutex;
    };

    Shard& getShard(const Key& key) {
        size_t idx = Hash{}(key) % m_segments;
        return m_shards[idx];
    }

    size_t m_capacity;
    size_t m_segments;
    std::chrono::seconds m_ttl;
    std::vector<Shard> m_shards;
    std::atomic<uint64_t> m_hits{0};
    std::atomic<uint64_t> m_misses{0};
};

} // namespace Cache
} // namespace NeonGlyph