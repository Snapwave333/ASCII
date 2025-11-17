#pragma once

#include "DirectorTypes.h"
#include <memory>
#include <string>

namespace NeonGlyph {

class DirectorClient {
public:
    virtual ~DirectorClient() = default;

    // Called from a worker thread: sends state, polls for new directives. 
    virtual void Pump() = 0;

    // Non-blocking: returns true if a directive is available. 
    virtual bool TryGetLatestDirective(DirectorDirective& outDir) = 0;
};

// Factory function to get a concrete implementation.
std::unique_ptr<DirectorClient> CreateDirectorClient(const std::string& host, int port);

} // namespace NeonGlyph