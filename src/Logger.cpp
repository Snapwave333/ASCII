#include "Logger.h"
#include <chrono>
#include <cstdlib>
#include <iostream>

namespace NeonGlyph {

std::mutex Logger::mtx;
std::ofstream Logger::ofs;
bool Logger::initialized = false;

static std::string now_ts() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return std::to_string(ms);
}

void Logger::Init(const std::string& path) {
    std::lock_guard<std::mutex> lock(mtx);
    if (!initialized) {
        ofs.open(path, std::ios::out | std::ios::app);
        initialized = ofs.is_open();
    }
}

void Logger::LogLine(const std::string& line) {
    try {
        std::lock_guard<std::mutex> lock(mtx);
        if (!initialized) {
            const char* env = std::getenv("NG_METRICS_PATH");
            std::string p = env ? std::string(env) : std::string("staging_run.log");
            ofs.open(p, std::ios::out | std::ios::app);
            initialized = ofs.is_open();
        }
        if (initialized && ofs.good()) {
            std::string s = std::string("TS=") + now_ts() + " " + line;
            ofs.write(s.c_str(), static_cast<std::streamsize>(s.size()));
            ofs.put('\n');
            ofs.flush();
        }
    } catch (const std::exception& e) {
        // Silently ignore logging errors to prevent crashes
        std::cerr << "[Logger] Exception in LogLine: " << e.what() << std::endl;
    } catch (...) {
        // Silently ignore any other errors
        std::cerr << "[Logger] Unknown exception in LogLine" << std::endl;
    }
}

void Logger::LogEvent(const std::string& category, const std::string& name, double ms) {
    LogLine(category + " " + name + "Ms=" + std::to_string(ms));
}

}

