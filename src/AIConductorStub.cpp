#include "DirectorTypes.h"
#include "AIConductor.h"

namespace NeonGlyph {

AIConductor::AIConductor() {}
AIConductor::~AIConductor() {}
Result AIConductor::Initialize(const Config&) { return Result::Success; }
void AIConductor::ApplyDirective(const DirectorDirective&) {}

}