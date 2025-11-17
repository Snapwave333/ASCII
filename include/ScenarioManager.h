#pragma once

#include "NeonGlyph.h"
#include <string>

namespace NeonGlyph {

class ScenarioManager {
public:
    Result Initialize(const Config& config);
    Result Generate(const std::string& songId);
    std::string GetHint(const std::string& songId, const std::string& section);
private:
    Config m_config;
    bool PostGenerate(const std::string& body, std::string& out);
};

}

