#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <cmath>

// Windows ICO file format structures
#pragma pack(push, 1)
struct ICONDIR {
    uint16_t idReserved;   // Reserved (must be 0)
    uint16_t idType;       // Resource type (1 for icons)
    uint16_t idCount;      // Number of images
};

struct ICONDIRENTRY {
    uint8_t  bWidth;       // Width (in pixels)
    uint8_t  bHeight;      // Height (in pixels)
    uint8_t  bColorCount;  // Number of colors (0 for 24-bit)
    uint8_t  bReserved;    // Reserved (must be 0)
    uint16_t wPlanes;      // Color planes
    uint16_t wBitCount;    // Bits per pixel
    uint32_t dwBytesInRes; // Size of image data (in bytes)
    uint32_t dwImageOffset;// Offset to image data
};

struct BITMAPINFOHEADER {
    uint32_t biSize;          // Size of header
    int32_t  biWidth;         // Width (in pixels)
    int32_t  biHeight;        // Height (in pixels, doubled for ICO)
    uint16_t biPlanes;        // Color planes
    uint16_t biBitCount;      // Bits per pixel
    uint32_t biCompression;   // Compression type (0 for none)
    uint32_t biSizeImage;     // Size of image data (0 for uncompressed)
    int32_t  biXPelsPerMeter;  // X pixels per meter
    int32_t  biYPelsPerMeter;  // Y pixels per meter
    uint32_t biClrUsed;       // Number of colors used
    uint32_t biClrImportant;  // Number of important colors
};
#pragma pack(pop)

class CyberpunkIconGenerator {
private:
    // Cyberpunk gradient colors
    struct Color {
        uint8_t r, g, b, a;
        Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
            : r(red), g(green), b(blue), a(alpha) {}
    };

    Color interpolateColor(const Color& c1, const Color& c2, float t) {
        return Color(
            static_cast<uint8_t>(c1.r + (c2.r - c1.r) * t),
            static_cast<uint8_t>(c1.g + (c2.g - c1.g) * t),
            static_cast<uint8_t>(c1.b + (c2.b - c1.b) * t),
            static_cast<uint8_t>(c1.a + (c2.a - c1.a) * t)
        );
    }

    Color getCyberpunkGradient(float x, float y, float centerX, float centerY) {
        // Cyberpunk gradient: cyan (#0FF0FC) to pink (#FF4D67)
        Color cyan(15, 240, 252);    // #0FF0FC
        Color pink(255, 77, 103);    // #FF4D67
        Color dark(13, 13, 13);      // #0D0D0D (background)
        
        float distance = std::sqrt((x - centerX) * (x - centerX) + (y - centerY) * (y - centerY));
        float maxDistance = std::sqrt(centerX * centerX + centerY * centerY);
        float t = distance / maxDistance;
        
        // Create radial gradient effect
        if (distance < centerX * 0.7f) {
            return interpolateColor(cyan, pink, t);
        } else {
            return interpolateColor(pink, dark, (t - 0.7f) / 0.3f);
        }
    }

    void drawAtSymbol(std::vector<uint8_t>& image, int width, int height, int x, int y, int size) {
        // Simple @ symbol drawing (pixel art style)
        // This is a simplified version - a real implementation would use proper font rendering
        
        Color glowColor(15, 240, 252, 180);  // Cyan with alpha for glow
        Color symbolColor(255, 77, 103);      // Pink for the @
        
        // Draw glow effect first (larger, more transparent)
        int glowSize = size + 4;
        for (int py = -glowSize/2; py < glowSize/2; py++) {
            for (int px = -glowSize/2; px < glowSize/2; px++) {
                int imgX = x + px;
                int imgY = y + py;
                if (imgX >= 0 && imgX < width && imgY >= 0 && imgY < height) {
                    float distance = std::sqrt(px*px + py*py);
                    if (distance < glowSize/2) {
                        float alpha = (1.0f - distance / (glowSize/2)) * 0.3f;
                        int idx = (imgY * width + imgX) * 4;
                        if (idx + 3 < image.size()) {
                            // Alpha blending
                            image[idx] = static_cast<uint8_t>(image[idx] * (1-alpha) + glowColor.r * alpha);
                            image[idx+1] = static_cast<uint8_t>(image[idx+1] * (1-alpha) + glowColor.g * alpha);
                            image[idx+2] = static_cast<uint8_t>(image[idx+2] * (1-alpha) + glowColor.b * alpha);
                            image[idx+3] = static_cast<uint8_t>(image[idx+3] * (1-alpha) + glowColor.a * alpha);
                        }
                    }
                }
            }
        }
        
        // Draw @ symbol (simplified circular shape with a line)
        for (int py = -size/2; py < size/2; py++) {
            for (int px = -size/2; px < size/2; px++) {
                int imgX = x + px;
                int imgY = y + py;
                if (imgX >= 0 && imgX < width && imgY >= 0 && imgY < height) {
                    float distance = std::sqrt(px*px + py*py);
                    
                    // Create circular @ shape
                    if (distance < size/2 && distance > size/4) {
                        // Outer circle
                        if (std::abs(distance - size/2) < 1.5f || std::abs(distance - size/4) < 1.0f) {
                            int idx = (imgY * width + imgX) * 4;
                            if (idx + 3 < image.size()) {
                                image[idx] = symbolColor.r;
                                image[idx+1] = symbolColor.g;
                                image[idx+2] = symbolColor.b;
                                image[idx+3] = symbolColor.a;
                            }
                        }
                    }
                    
                    // Inner "a" shape (simplified)
                    if (px > -size/6 && px < size/6 && py > -size/8 && py < size/4) {
                        int idx = (imgY * width + imgX) * 4;
                        if (idx + 3 < image.size()) {
                            image[idx] = symbolColor.r;
                            image[idx+1] = symbolColor.g;
                            image[idx+2] = symbolColor.b;
                            image[idx+3] = symbolColor.a;
                        }
                    }
                }
            }
        }
    }

    void drawRoundedRectangle(std::vector<uint8_t>& image, int width, int height, 
                             int x, int y, int w, int h, int radius, const Color& color) {
        for (int py = 0; py < h; py++) {
            for (int px = 0; px < w; px++) {
                int imgX = x + px;
                int imgY = y + py;
                
                if (imgX >= 0 && imgX < width && imgY >= 0 && imgY < height) {
                    // Check if pixel is inside rounded rectangle
                    float distX = std::min(std::abs(px - radius), std::abs(px - (w - radius)));
                    float distY = std::min(std::abs(py - radius), std::abs(py - (h - radius)));
                    
                    if (px < radius && py < radius) {
                        // Top-left corner
                        float dist = std::sqrt((px - radius) * (px - radius) + (py - radius) * (py - radius));
                        if (dist > radius) continue;
                    } else if (px >= w - radius && py < radius) {
                        // Top-right corner
                        float dist = std::sqrt((px - (w - radius)) * (px - (w - radius)) + (py - radius) * (py - radius));
                        if (dist > radius) continue;
                    } else if (px < radius && py >= h - radius) {
                        // Bottom-left corner
                        float dist = std::sqrt((px - radius) * (px - radius) + (py - (h - radius)) * (py - (h - radius)));
                        if (dist > radius) continue;
                    } else if (px >= w - radius && py >= h - radius) {
                        // Bottom-right corner
                        float dist = std::sqrt((px - (w - radius)) * (px - (w - radius)) + (py - (h - radius)) * (py - (h - radius)));
                        if (dist > radius) continue;
                    }
                    
                    int idx = (imgY * width + imgX) * 4;
                    if (idx + 3 < image.size()) {
                        image[idx] = color.r;
                        image[idx+1] = color.g;
                        image[idx+2] = color.b;
                        image[idx+3] = color.a;
                    }
                }
            }
        }
    }

public:
    std::vector<uint8_t> generateIcon(int size) {
        std::vector<uint8_t> image(size * size * 4, 0);  // RGBA
        
        // Fill with dark background
        Color darkBg(13, 13, 13, 255);  // #0D0D0D
        for (int i = 0; i < size * size; i++) {
            image[i * 4] = darkBg.r;
            image[i * 4 + 1] = darkBg.g;
            image[i * 4 + 2] = darkBg.b;
            image[i * 4 + 3] = darkBg.a;
        }
        
        // Draw rounded rectangle border
        int borderWidth = size < 32 ? 2 : 4;
        Color borderColor(15, 240, 252, 255);  // Cyan
        drawRoundedRectangle(image, size, size, 0, 0, size, size, size/8, borderColor);
        
        // Draw inner background
        Color innerBg(13, 13, 13, 255);  // #0D0D0D
        drawRoundedRectangle(image, size, size, borderWidth, borderWidth, 
                           size - borderWidth * 2, size - borderWidth * 2, size/8 - borderWidth, innerBg);
        
        // Draw cyberpunk gradient background
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                // Skip border areas
                if (x < borderWidth || x >= size - borderWidth || 
                    y < borderWidth || y >= size - borderWidth) continue;
                
                Color gradientColor = getCyberpunkGradient(x, y, size/2, size/2);
                int idx = (y * size + x) * 4;
                
                // Alpha blend with background
                float alpha = 0.3f;  // Subtle gradient
                image[idx] = static_cast<uint8_t>(image[idx] * (1-alpha) + gradientColor.r * alpha);
                image[idx+1] = static_cast<uint8_t>(image[idx+1] * (1-alpha) + gradientColor.g * alpha);
                image[idx+2] = static_cast<uint8_t>(image[idx+2] * (1-alpha) + gradientColor.b * alpha);
            }
        }
        
        // Draw @ symbol in center
        int atSize = size < 32 ? size/3 : size/2;
        drawAtSymbol(image, size, size, size/2, size/2, atSize);
        
        return image;
    }

    bool writeICO(const std::string& filename, const std::vector<int>& sizes) {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return false;
        }

        // Write icon directory
        ICONDIR iconDir;
        iconDir.idReserved = 0;
        iconDir.idType = 1;  // Icon type
        iconDir.idCount = static_cast<uint16_t>(sizes.size());
        
        file.write(reinterpret_cast<const char*>(&iconDir), sizeof(iconDir));

        // Generate images and write directory entries
        std::vector<std::vector<uint8_t>> images;
        std::vector<uint32_t> offsets;
        uint32_t currentOffset = sizeof(ICONDIR) + sizes.size() * sizeof(ICONDIRENTRY);

        for (int size : sizes) {
            std::vector<uint8_t> image = generateIcon(size);
            images.push_back(image);
            
            // Write directory entry
            ICONDIRENTRY entry;
            entry.bWidth = static_cast<uint8_t>(size);
            entry.bHeight = static_cast<uint8_t>(size);
            entry.bColorCount = 0;  // 24-bit true color
            entry.bReserved = 0;
            entry.wPlanes = 1;
            entry.wBitCount = 32;  // 32-bit RGBA
            entry.dwBytesInRes = sizeof(BITMAPINFOHEADER) + size * size * 4 + size * size;  // Header + RGBA + mask
            entry.dwImageOffset = currentOffset;
            
            file.write(reinterpret_cast<const char*>(&entry), sizeof(entry));
            currentOffset += entry.dwBytesInRes;
        }

        // Write image data
        for (size_t i = 0; i < images.size(); i++) {
            int size = sizes[i];
            const auto& image = images[i];
            
            // Write bitmap info header
            BITMAPINFOHEADER bih;
            bih.biSize = sizeof(BITMAPINFOHEADER);
            bih.biWidth = size;
            bih.biHeight = size * 2;  // Height doubled for ICO format
            bih.biPlanes = 1;
            bih.biBitCount = 32;
            bih.biCompression = 0;
            bih.biSizeImage = 0;
            bih.biXPelsPerMeter = 0;
            bih.biYPelsPerMeter = 0;
            bih.biClrUsed = 0;
            bih.biClrImportant = 0;
            
            file.write(reinterpret_cast<const char*>(&bih), sizeof(bih));
            
            // Write RGBA data (upside down for Windows BMP format)
            for (int y = size - 1; y >= 0; y--) {
                for (int x = 0; x < size; x++) {
                    int idx = (y * size + x) * 4;
                    file.write(reinterpret_cast<const char*>(&image[idx]), 4);  // BGRA (little-endian)
                }
            }
            
            // Write empty mask (no transparency mask needed for 32-bit)
            std::vector<uint8_t> mask(size * size / 8, 0);
            file.write(reinterpret_cast<const char*>(mask.data()), mask.size());
        }

        file.close();
        return true;
    }
};

int main() {
    std::cout << "NeonGlyph Cyberpunk Icon Generator" << std::endl;
    std::cout << "==================================" << std::endl;
    
    CyberpunkIconGenerator generator;
    
    // Generate different icon sizes
    std::vector<int> iconSizes = {16, 24, 32, 48, 64, 128, 256};
    
    std::cout << "Generating icons in sizes: ";
    for (int size : iconSizes) {
        std::cout << size << "x" << size << " ";
    }
    std::cout << std::endl;
    
    // Generate main application icon
    std::cout << "Generating main application icon..." << std::endl;
    if (generator.writeICO("neonglyph-icon.ico", iconSizes)) {
        std::cout << "✓ Generated neonglyph-icon.ico" << std::endl;
    } else {
        std::cerr << "✗ Failed to generate main icon" << std::endl;
        return 1;
    }
    
    // Generate tray icon (smaller sizes)
    std::vector<int> traySizes = {16, 24, 32};
    std::cout << "Generating system tray icon..." << std::endl;
    if (generator.writeICO("neonglyph-tray-icon.ico", traySizes)) {
        std::cout << "✓ Generated neonglyph-tray-icon.ico" << std::endl;
    } else {
        std::cerr << "✗ Failed to generate tray icon" << std::endl;
        return 1;
    }
    
    // Generate favicon
    std::vector<int> faviconSizes = {16, 32, 48};
    std::cout << "Generating favicon..." << std::endl;
    if (generator.writeICO("neonglyph-favicon.ico", faviconSizes)) {
        std::cout << "✓ Generated neonglyph-favicon.ico" << std::endl;
    } else {
        std::cerr << "✗ Failed to generate favicon" << std::endl;
        return 1;
    }
    
    std::cout << std::endl;
    std::cout << "Icon generation complete!" << std::endl;
    std::cout << "Files created:" << std::endl;
    std::cout << "  - neonglyph-icon.ico (main application icon)" << std::endl;
    std::cout << "  - neonglyph-tray-icon.ico (system tray icon)" << std::endl;
    std::cout << "  - neonglyph-favicon.ico (browser favicon)" << std::endl;
    
    return 0;
}