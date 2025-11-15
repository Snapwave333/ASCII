#include "IconGenerator.h"
#include <cstring>
#include <algorithm>
#include <cmath>

namespace NeonGlyph {

std::vector<uint8_t> IconGenerator::GenerateIcon(int width, int height) {
    std::vector<uint8_t> rgba;
    rgba.resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 4u);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            size_t index = static_cast<size_t>(y) * static_cast<size_t>(width) * 4u + static_cast<size_t>(x) * 4u;
            float cx = width * 0.5f;
            float cy = height * 0.5f;
            float r = std::min(width, height) * 0.33f;
            float dx = x - cx;
            float dy = y - cy;
            float d = std::sqrt(dx * dx + dy * dy);
            bool ring = d > r * 0.3f && d < r;
            bool bar = std::abs(dx) < r * 0.2f && dy > 0;
            uint8_t rr = 0, gg = 0, bb = 0, aa = 0;
            if (ring || bar) {
                float t = static_cast<float>(x) / static_cast<float>(width);
                rr = static_cast<uint8_t>(255.0f * t);
                gg = static_cast<uint8_t>(255.0f * (1.0f - t));
                bb = 204;
                aa = 255;
            }
            rgba[index + 0] = bb;
            rgba[index + 1] = gg;
            rgba[index + 2] = rr;
            rgba[index + 3] = aa;
        }
    }
    BitmapInfoHeader hdr{};
    hdr.size = sizeof(BitmapInfoHeader);
    hdr.width = width;
    hdr.height = height * 2;
    hdr.planes = 1;
    hdr.bitCount = 32;
    hdr.compression = 0;
    hdr.sizeImage = static_cast<uint32_t>(width * height * 4 + (width * height / 8));
    std::vector<uint8_t> icoImage;
    icoImage.resize(sizeof(BitmapInfoHeader) + rgba.size() + (static_cast<size_t>(width) * static_cast<size_t>(height) / 8u));
    std::memcpy(icoImage.data(), &hdr, sizeof(hdr));
    std::memcpy(icoImage.data() + sizeof(hdr), rgba.data(), rgba.size());
    std::memset(icoImage.data() + sizeof(hdr) + rgba.size(), 0, static_cast<size_t>(width) * static_cast<size_t>(height) / 8u);
    return icoImage;
}

std::vector<uint8_t> IconGenerator::GenerateMultiSizeIcon(const std::vector<int>& sizes) {
    IcoHeader hdr{};
    hdr.reserved = 0;
    hdr.type = 1;
    hdr.count = static_cast<uint16_t>(sizes.size());
    std::vector<IcoEntry> entries;
    std::vector<std::vector<uint8_t>> images;
    entries.resize(sizes.size());
    images.resize(sizes.size());
    for (size_t i = 0; i < sizes.size(); ++i) {
        int s = sizes[i];
        images[i] = GenerateIcon(s, s);
        IcoEntry e{};
        e.width = (s == 256) ? 0 : static_cast<uint8_t>(s);
        e.height = (s == 256) ? 0 : static_cast<uint8_t>(s);
        e.colorCount = 0;
        e.reserved = 0;
        e.planes = 1;
        e.bitCount = 32;
        e.size = static_cast<uint32_t>(images[i].size());
        entries[i] = e;
    }
    size_t offset = sizeof(IcoHeader) + entries.size() * sizeof(IcoEntry);
    for (size_t i = 0; i < entries.size(); ++i) {
        entries[i].offset = static_cast<uint32_t>(offset);
        offset += images[i].size();
    }
    std::vector<uint8_t> file;
    file.resize(sizeof(IcoHeader));
    std::memcpy(file.data(), &hdr, sizeof(hdr));
    for (const auto& e : entries) {
        size_t p = file.size();
        file.resize(p + sizeof(IcoEntry));
        std::memcpy(file.data() + p, &e, sizeof(IcoEntry));
    }
    for (const auto& img : images) {
        size_t p = file.size();
        file.resize(p + img.size());
        std::memcpy(file.data() + p, img.data(), img.size());
    }
    return file;
}

} // namespace NeonGlyph