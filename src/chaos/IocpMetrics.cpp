#include "chaos/IocpMetrics.h"

namespace NeonGlyph { namespace Chaos {

std::atomic<uint64_t> IocpMetrics::recv_{0};
std::atomic<uint64_t> IocpMetrics::send_{0};

} }