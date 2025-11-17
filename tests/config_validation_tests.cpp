#include "NeonGlyph.h"
#include <iostream>

using namespace NeonGlyph;

int main() {
    Config cfg;
    cfg.headless.enabled = true;
    cfg.window.fullscreen = true;
    ConfigManager cm;
    std::string msg;
    Result r = cm.ValidateConfig(cfg, msg);
    if (r == Result::ValidationFailed) {
        std::cout << "OK:" << msg << std::endl;
        return 0;
    }
    std::cout << "FAIL" << std::endl;
    return 1;
}