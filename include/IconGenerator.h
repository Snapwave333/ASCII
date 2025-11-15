// Simple ICO file header structure for creating placeholder icons
// This is a header file that defines the ICO file format for manual creation

#pragma once

#include <cstdint>
#include <vector>

namespace NeonGlyph {

// ICO file format structures
#pragma pack(push, 1)

struct IcoHeader {
    uint16_t reserved;      // Reserved (must be 0)
    uint16_t type;         // Resource type (1 for icons)
    uint16_t count;        // Number of images
};

struct IcoEntry {
    uint8_t width;         // Image width (0 means 256)
    uint8_t height;        // Image height (0 means 256)
    uint8_t colorCount;    // Number of colors (0 if no palette)
    uint8_t reserved;      // Reserved (must be 0)
    uint16_t planes;      // Color planes
    uint16_t bitCount;     // Bits per pixel
    uint32_t size;         // Size of image data in bytes
    uint32_t offset;       // Offset of image data from beginning of file
};

struct BitmapInfoHeader {
    uint32_t size;         // Size of this header (40 bytes)
    int32_t width;         // Image width in pixels
    int32_t height;        // Image height in pixels (2x icon height for AND mask)
    uint16_t planes;       // Number of color planes (must be 1)
    uint16_t bitCount;     // Bits per pixel
    uint32_t compression;  // Compression type (0 for uncompressed)
    uint32_t sizeImage;    // Size of image data
    int32_t xPelsPerMeter; // Horizontal resolution
    int32_t yPelsPerMeter; // Vertical resolution
    uint32_t clrUsed;      // Number of colors in color table
    uint32_t clrImportant; // Number of important colors
};

#pragma pack(pop)

// Simple placeholder icon generator
class PlaceholderIconGenerator {
public:
    static std::vector<uint8_t> GeneratePlaceholderIcon(int width, int height);
    static std::vector<uint8_t> GenerateMultiSizeIcon(const std::vector<int>& sizes);
    
private:
    static std::vector<uint8_t> GeneratePNGData(int width, int height);
    static std::vector<uint8_t> GenerateBMPData(int width, int height);
    static std::vector<uint8_t> GenerateANDMask(int width, int height);
};

} // namespace NeonGlyph