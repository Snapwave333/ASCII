#include "cache/CacheMonitor.h"

namespace NeonGlyph { namespace Cache {

std::unordered_map<std::string, std::pair<std::atomic<uint64_t>, std::atomic<uint64_t>>> CacheMonitor::map_;
std::mutex CacheMonitor::mutex_;

} }