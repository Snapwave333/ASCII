#pragma once

#include <map>
#include <string>
#include <tuple>

namespace NeonGlyph { namespace Production {

class PerformanceMonitor;

class PerfMonitorRegistry {
public:
    static void Register(PerformanceMonitor* m);
    static PerformanceMonitor* Get();
};

} }