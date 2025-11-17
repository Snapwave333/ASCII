#include "Renderer.h"
#include "NeonGlyph.h"
#include <iostream>
#include <cassert>

using namespace NeonGlyph;

static void TestSingleRedPixel() {
    Renderer r;
    Config cfg;
    r.Initialize(nullptr, cfg);
    r.ResizeCanvas(1, 1);
    r.ClearCanvas(' ');
    r.SetCell(0, 0, '#', 0xFF0000u);
    auto px = r.ComposeFramePixels(1, 1);
    assert(px.size() == 4);
    assert(px[0] == 0 && px[1] == 0 && px[2] == 255 && px[3] == 255);
}

static void TestColorChannelsStrip() {
    Renderer r;
    Config cfg;
    r.Initialize(nullptr, cfg);
    r.ResizeCanvas(4, 1);
    r.ClearCanvas(' ');
    r.SetCell(0, 0, 'R', 0xFF0000u);
    r.SetCell(1, 0, 'G', 0x00FF00u);
    r.SetCell(2, 0, 'B', 0x0000FFu);
    r.SetCell(3, 0, 'W', 0xFFFFFFu);
    auto px = r.ComposeFramePixels(4, 1);
    assert(px.size() == 16);
    // Red
    assert(px[0] == 0 && px[1] == 0 && px[2] == 255 && px[3] == 255);
    // Green
    assert(px[4] == 0 && px[5] == 255 && px[6] == 0 && px[7] == 255);
    // Blue
    assert(px[8] == 255 && px[9] == 0 && px[10] == 0 && px[11] == 255);
    // White
    assert(px[12] == 255 && px[13] == 255 && px[14] == 255 && px[15] == 255);
}

int main() {
    TestSingleRedPixel();
    TestColorChannelsStrip();
    std::cout << "PASS: bgra packing and channel correctness" << std::endl;
    return 0;
}