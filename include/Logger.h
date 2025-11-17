#pragma once

#include <string>
#include <mutex>
#include <fstream>

namespace NeonGlyph {

class Logger {
public:
    static void Init(const std::string& path);
    static void LogLine(const std::string& line);
    static void LogEvent(const std::string& category, const std::string& name, double ms);
private:
    static std::mutex mtx;
    static std::ofstream ofs;
    static bool initialized;
};

}

