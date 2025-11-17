#pragma once

#include <atomic>
#include <algorithm>

namespace NeonGlyph { namespace Chaos {

class IocpMetrics {
public:
    static void IncRecv() { recv_.fetch_add(1, std::memory_order_relaxed); }
    static void IncSend() { send_.fetch_add(1, std::memory_order_relaxed); }
    static double DispersionScore() {
        auto r = recv_.load(std::memory_order_relaxed);
        auto s = send_.load(std::memory_order_relaxed);
        if (r + s == 0) return 1.0;
        double ratio = (double)std::min(r, s) / (double)std::max(r, s);
        return ratio; // closer to 1 is more balanced
    }
private:
    static std::atomic<uint64_t> recv_;
    static std::atomic<uint64_t> send_;
};

} }