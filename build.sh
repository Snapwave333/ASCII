#!/bin/bash

echo "PROJECT NEON-GLYPH Build System"
echo "==============================="

# Check for required tools
check_tool() {
    if ! command -v $1 &> /dev/null; then
        echo "ERROR: $1 not found!"
        echo "Please install $1 and add it to your PATH"
        exit 1
    fi
}

# Check dependencies
check_tool cmake
check_tool g++

# Check for Vulkan SDK
if [ -z "$VULKAN_SDK" ]; then
    echo "ERROR: Vulkan SDK not found!"
    echo "Please install Vulkan SDK and set VULKAN_SDK environment variable"
    exit 1
fi

# Create build directory
mkdir -p build
cd build

# Configure with CMake
echo "Configuring project..."
cmake -DCMAKE_BUILD_TYPE=Release ..
if [ $? -ne 0 ]; then
    echo "ERROR: CMake configuration failed!"
    exit 1
fi

# Build the project
echo "Building project..."
make -j$(nproc)
if [ $? -ne 0 ]; then
    echo "ERROR: Build failed!"
    exit 1
fi

echo ""
echo "Build completed successfully!"
echo "Executable: build/NeonGlyph"
echo ""
echo "To run the application:"
echo "  ./NeonGlyph"

# Return to root directory
cd ..