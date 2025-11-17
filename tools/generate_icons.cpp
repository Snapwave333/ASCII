#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cmath>

// Simple BMP/ICO file generator for NeonGlyph branding
// This creates actual binary ICO files, not placeholders

struct BMPHeader {
    uint16_t signature = 0x4D42; // "BM"
    uint32_t fileSize;
    uint32_t reserved = 0;
    uint32_t dataOffset = 54; // Standard BMP header size
    
    // Info header
    uint32_t headerSize = 40;
    int32_t width;
    int32_t height;
    uint16_t planes = 1;
    uint16_t bitCount = 32;
    uint32_t compression = 0;
    uint32_t imageSize;
    int32_t xPelsPerMeter = 0;
    int32_t yPelsPerMeter = 0;
    uint32_t clrUsed = 0;
    uint32_t clrImportant = 0;
};

struct ICOHeader {
    uint16_t reserved = 0;
    uint16_t type = 1; // Icon
    uint16_t count;
};

struct ICOEntry {
    uint8_t width;
    uint8_t height;
    uint8_t colorCount = 0;
    uint8_t reserved = 0;
    uint16_t planes = 1;
    uint16_t bitCount = 32;
    uint32_t size;
    uint32_t offset;
};

// Color structure for our cyberpunk gradient
struct Color {
    uint8_t b, g, r, a;
    
    Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
        : b(blue), g(green), r(red), a(alpha) {}
};

// Generate cyberpunk gradient colors
Color getGradientColor(float t) {
    // Cyberpunk gradient: cyan (#0FF0FC) to magenta (#FF4D67)
    uint8_t r = static_cast<uint8_t>(15 + t * (255 - 15));
    uint8_t g = static_cast<uint8_t>(240 * (1.0f - t * 0.8f));
    uint8_t b = static_cast<uint8_t>(252 * (1.0f - t * 0.7f));
    return Color(r, g, b, 255);
}

// Create @ symbol pattern
bool isAtSymbol(int x, int y, int size) {
    float centerX = size / 2.0f;
    float centerY = size / 2.0f;
    float radius = size / 3.0f;
    
    float dx = x - centerX;
    float dy = y - centerY;
    float dist = std::sqrt(dx*dx + dy*dy);
    
    // Outer circle
    bool outerCircle = (dist > radius * 0.6f && dist < radius);
    
    // Inner "hole" - make it smaller
    bool innerHole = (dist < radius * 0.3f);
    
    // Vertical line (tail of @)
    bool verticalLine = (std::abs(dx) < radius * 0.15f && dy > 0 && dy < radius * 0.7f);
    
    return (outerCircle && !innerHole) || verticalLine;
}

// Generate BMP image data for @ symbol
std::vector<Color> generateAtSymbolImage(int size) {
    std::vector<Color> pixels(size * size);
    
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int index = y * size + x;
            
            // Background (transparent)
            pixels[index] = Color(0, 0, 0, 0);
            
            // Check if this pixel should be part of the @ symbol
            if (isAtSymbol(x, y, size)) {
                // Create gradient effect based on position
                float gradientX = static_cast<float>(x) / size;
                pixels[index] = getGradientColor(gradientX);
            }
        }
    }
    
    return pixels;
}

// Write BMP file (for individual icon sizes)
void writeBMPFile(const std::string& filename, int size, const std::vector<Color>& pixels) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to create " << filename << std::endl;
        return;
    }
    
    BMPHeader header;
    header.width = size;
    header.height = size;
    header.imageSize = size * size * 4; // 32-bit RGBA
    header.fileSize = header.dataOffset + header.imageSize;
    
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    
    // Write pixel data (BMP stores bottom-to-top)
    for (int y = size - 1; y >= 0; y--) {
        for (int x = 0; x < size; x++) {
            int index = y * size + x;
            file.write(reinterpret_cast<const char*>(&pixels[index]), sizeof(Color));
        }
    }
    
    file.close();
    std::cout << "Created " << filename << " (" << size << "x" << size << ")" << std::endl;
}

// Create ICO file with multiple sizes
void createICOFile(const std::string& filename, const std::vector<int>& sizes) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to create " << filename << std::endl;
        return;
    }
    
    // Generate all image data first
    std::vector<std::vector<Color>> allPixels;
    std::vector<std::vector<uint8_t>> allBMPData;
    
    for (int size : sizes) {
        auto pixels = generateAtSymbolImage(size);
        allPixels.push_back(pixels);
        
        // Convert to BMP format
        std::vector<uint8_t> bmpData;
        
        // BMP header
        BMPHeader header;
        header.width = size;
        header.height = size * 2; // Height includes AND mask
        header.imageSize = size * size * 4;
        header.fileSize = sizeof(BMPHeader) + header.imageSize + (size * size / 8); // Add AND mask size
        
        // Write header to temporary buffer
        std::vector<uint8_t> headerBytes(sizeof(BMPHeader));
        std::memcpy(headerBytes.data(), &header, sizeof(header));
        bmpData.insert(bmpData.end(), headerBytes.begin(), headerBytes.end());
        
        // Write pixel data (bottom-to-top for BMP)
        for (int y = size - 1; y >= 0; y--) {
            for (int x = 0; x < size; x++) {
                int index = y * size + x;
                const Color& color = pixels[index];
                bmpData.push_back(color.b);
                bmpData.push_back(color.g);
                bmpData.push_back(color.r);
                bmpData.push_back(color.a);
            }
        }
        
        // Add AND mask (all 0 for full transparency)
        int maskSize = size * size / 8;
        bmpData.resize(bmpData.size() + maskSize, 0);
        
        allBMPData.push_back(bmpData);
    }
    
    // Write ICO header
    ICOHeader icoHeader;
    icoHeader.count = static_cast<uint16_t>(sizes.size());
    file.write(reinterpret_cast<const char*>(&icoHeader), sizeof(icoHeader));
    
    // Write directory entries
    uint32_t currentOffset = sizeof(ICOHeader) + (sizeof(ICOEntry) * sizes.size());
    
    for (size_t i = 0; i < sizes.size(); i++) {
        ICOEntry entry;
        int size = sizes[i];
        entry.width = (size == 256) ? 0 : static_cast<uint8_t>(size);
        entry.height = (size == 256) ? 0 : static_cast<uint8_t>(size);
        entry.size = static_cast<uint32_t>(allBMPData[i].size());
        entry.offset = currentOffset;
        
        file.write(reinterpret_cast<const char*>(&entry), sizeof(entry));
        currentOffset += entry.size;
    }
    
    // Write image data
    for (const auto& bmpData : allBMPData) {
        file.write(reinterpret_cast<const char*>(bmpData.data()), bmpData.size());
    }
    
    file.close();
    std::cout << "Created " << filename << " with " << sizes.size() << " icon sizes" << std::endl;
}

int main() {
    std::cout << "Generating NeonGlyph ICO files..." << std::endl;
    
    // Create main application icon with multiple sizes
    std::vector<int> mainIconSizes = {16, 24, 32, 48, 64, 128, 256};
    createICOFile("neonglyph-icon-256.ico", mainIconSizes);
    
    // Create tray icon with smaller sizes
    std::vector<int> trayIconSizes = {16, 24, 32};
    createICOFile("neonglyph-tray-icon.ico", trayIconSizes);
    
    std::cout << "ICO generation complete!" << std::endl;
    return 0;
}