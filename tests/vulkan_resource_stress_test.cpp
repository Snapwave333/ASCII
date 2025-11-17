#include "VulkanContext.h"
#include <iostream>

using namespace NeonGlyph;

int main() {
    for (int i = 0; i < 10; ++i) {
        VulkanContext vc;
        Config cfg;
        Result r = vc.Initialize(cfg);
        if (r != Result::Success) {
            std::cout << "INIT_FAIL" << std::endl;
            return 1;
        }
        vc.Shutdown();
    }
    std::cout << "OK" << std::endl;
    return 0;
}