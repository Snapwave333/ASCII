#include "NeonGlyph.h"
#include "VulkanContext.h"
#include "ASCIIConverter.h"
#include <iostream>

using namespace NeonGlyph;

int main() {
    Config cfg;
    VulkanContext vc;
    Result r0 = vc.Initialize(cfg);
    if (r0 != Result::Success) {
        std::cout << "SKIP: vulkan init" << std::endl;
        return 0;
    }
    ASCIIConverter ac;
    Result r1 = ac.Initialize(&vc, cfg);
    bool ok = (r1 == Result::Success);
    std::cout << (ok ? "PASS" : "FAIL") << ": font atlas fallback" << std::endl;
    return ok ? 0 : 1;
}

