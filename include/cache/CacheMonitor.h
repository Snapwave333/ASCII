#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <utility>

namespace NeonGlyph {
namespace Cache {

class CacheMonitor {
public:
    static void Update(const std::string& name, uint64_t hits, uint64_t misses) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& c = map_[name];
        c.first.store(hits, std::memory_order_relaxed);
        c.second.store(misses, std::memory_order_relaxed);
    }
    static std::pair<uint64_t,uint64_t> Stats(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = map_.find(name);
        if (it == map_.end()) return {0,0};
        return { it->second.first.load(std::memory_order_relaxed), it->second.second.load(std::memory_order_relaxed) };
    }
private:
    static std::unordered_map<std::string, std::pair<std::atomic<uint64_t>, std::atomic<uint64_t>>> map_;
    static std::mutex mutex_;
};

} // namespace Cache
} // namespace NeonGlyph