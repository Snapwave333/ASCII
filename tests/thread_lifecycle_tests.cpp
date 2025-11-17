#include "chaos/FailureModeAnalysis.h"
#include <iostream>
#include <thread>

using namespace NeonGlyph::Chaos;

int main() {
    FailureModeAnalysis fma;
    fma.StartHealthMonitoring();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    fma.StopHealthMonitoring();
    std::cout << "OK" << std::endl;
    return 0;
}