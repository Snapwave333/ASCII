#!/usr/bin/env python3
"""
Simple ICO file generator for NeonGlyph branding
Creates actual Windows ICO files with cyberpunk @ symbol
"""

import struct
import math
import os

def create_simple_ico():
    """Create a simple ICO file with basic cyberpunk design"""
    
    # Create tools directory
    os.makedirs('tools', exist_ok=True)
    os.makedirs('assets/branding', exist_ok=True)
    
    # 32x32 icon data (simplified)
    size = 32
    
    # Create BMP pixel data (32x32 RGBA)
    pixels = bytearray()
    for y in range(size):
        for x in range(size):
            # Simple @ symbol pattern
            center_x, center_y = size//2, size//2
            distance = math.sqrt((x-center_x)**2 + (y-center_y)**2)
            
            if distance < 8:  # Inner @ area
                # Pink color #FF4D67
                pixels.extend([103, 77, 255, 255])  # BGRA
            elif distance < 12:  # Outer glow
                # Cyan color #0FF0FC
                pixels.extend([252, 240, 15, 128])  # BGRA with alpha
            else:
                # Dark background #0D0D0D
                pixels.extend([13, 13, 13, 255])  # BGRA
    
    # Create ICO file
    with open('tools/neonglyph-icon.ico', 'wb') as f:
        # ICONDIR header
        f.write(struct.pack('<HHH', 0, 1, 1))  # Reserved, Type, Count
        
        # ICONDIRENTRY
        f.write(struct.pack('<BBBBHHLL',
                           32, 32,  # Width, Height
                           0, 0,    # ColorCount, Reserved
                           1, 32,   # Planes, BitCount
                           40 + 32*32*4 + 32*32//8,  # Size
                           6 + 16))  # Offset
        
        # BMP info header
        f.write(struct.pack('<LLLHHLLLLLL',
                           40,  # Header size
                           32, 64,  # Width, Height (doubled)
                           1, 32,   # Planes, BitCount
                           0,       # Compression
                           32*32*4, # Image size
                           0, 0, 0, 0))  # Other fields
        
        # Pixel data (upside down for BMP)
        for y in range(31, -1, -1):
            f.write(pixels[y*32*4:(y+1)*32*4])
        
        # Mask data (empty)
        f.write(b'\x00' * (32*32//8))
    
    print("✓ Generated neonglyph-icon.ico")
    
    # Copy to assets
    import shutil
    shutil.copy('tools/neonglyph-icon.ico', 'assets/branding/neonglyph-icon.ico')
    
    return True

if __name__ == "__main__":
    print("NeonGlyph ICO Generator")
    print("=======================")
    create_simple_ico()
    print("Icon generation complete!")