#include "production/PerfMonitorRegistry.h"
#include "production/PerformanceManagementSystem.h"

namespace NeonGlyph { namespace Production {

static PerformanceMonitor* g_perfMon = nullptr;

void PerfMonitorRegistry::Register(PerformanceMonitor* m) { g_perfMon = m; }
PerformanceMonitor* PerfMonitorRegistry::Get() { return g_perfMon; }

} }