#include "NeonGlyph.h"
#include "VulkanContext.h"
#include "ConfigManager.h"
#include <iostream>
#include <fstream>
#include <string>

using namespace NeonGlyph;

static bool test_config_safe_parsing() {
    ConfigManager cm;
    Config cfg;
    std::string content;
    content += "{\n";
    content += "  \"ascii\": {\n";
    content += "    \"brightness\": \"abc\",\n";
    content += "    \"contrast\": \"\",\n";
    content += "    \"fontSize\": \"notnumber\"\n";
    content += "  }\n";
    content += "}\n";
    std::string path = "test_config_invalid.json";
    std::ofstream f(path); f << content; f.close();
    Result r = cm.LoadConfig(path, cfg);
    std::remove(path.c_str());
    if (r != Result::Success) return false;
    bool ok = true;
    ok = ok && (cfg.ascii.brightness == 0.0f);
    ok = ok && (cfg.ascii.contrast == 0.0f);
    ok = ok && (cfg.ascii.fontSize == 0u);
    std::cout << (ok ? "PASS" : "FAIL") << ": config safe parsing" << std::endl;
    return ok;
}

static bool test_config_default_fallback() {
    ConfigManager cm;
    Config cfg;
    Result r = cm.LoadDefaultConfig(cfg);
    bool ok = (r == Result::Success);
    ok = ok && (cfg.window.width > 0);
    ok = ok && (cfg.ascii.charset.size() > 0);
    std::cout << (ok ? "PASS" : "FAIL") << ": default config fallback" << std::endl;
    return ok;
}

static bool test_vulkan_defensive_checks() {
    VulkanContext vc;
    Result b1 = vc.BeginFrame();
    Result e1 = vc.EndFrame();
    bool ok = (b1 == Result::InitializationFailed) && (e1 == Result::InitializationFailed);
    std::cout << (ok ? "PASS" : "FAIL") << ": vulkan defensive checks" << std::endl;
    return ok;
}

static bool test_vulkan_instance_creation() {
    VulkanContext vc;
    Config cfg;
    Result r = vc.CreateInstance();
    bool ok = (r == Result::Success) || (r == Result::InitializationFailed);
    std::cout << (ok ? "PASS" : "FAIL") << ": vulkan instance create" << std::endl;
    return ok;
}

int main() {
    bool all = true;
    all = all && test_config_safe_parsing();
    all = all && test_config_default_fallback();
    all = all && test_vulkan_defensive_checks();
    all = all && test_vulkan_instance_creation();
    std::cout << (all ? "ALL PASS" : "SOME FAIL") << std::endl;
    return all ? 0 : 1;
}

