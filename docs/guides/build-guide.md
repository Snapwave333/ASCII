# NeonGlyph Build Guide

**Document Type**: Build Guide
**Version**: 2.1.0
**Last Updated**: 2025-11-13
**Status**: Approved
**Author**: NeonGlyph Development Team

## Overview

This guide provides comprehensive instructions for building the NeonGlyph AI-driven ASCII visual synthesis engine from source code. **Now featuring psychedelic DMT/acid trip visual generation with AI-powered consciousness-expanding experiences.**

## Prerequisites

### System Requirements
- **Operating System**: Windows 10 or later (64-bit)
- **Processor**: Intel Core i5 or AMD equivalent (minimum)
- **Memory**: 8GB RAM (16GB recommended)
- **Storage**: 2GB free disk space
- **Graphics**: DirectX 12 compatible GPU with Vulkan support

### Development Tools
- **Visual Studio 2022** (Community, Professional, or Enterprise)
  - Desktop development with C++ workload
  - Windows 10 SDK (latest version)
- **CMake 3.20** or higher
- **Git** for version control
- **Vulkan SDK** (latest version)

### Required SDKs and Libraries
- Vulkan SDK (install from [LunarG](https://vulkan.lunarg.com/))
- Windows 10 SDK (included with Visual Studio)
- C++20 compatible compiler (MSVC v143 or later)
- CMake 3.20 or higher
- Git for version control

### Optional Dependencies
- **ONNX Runtime**: For AI Director functionality (auto-detected)
- **FreeType**: For custom font support (auto-detected)
- **GLFW**: For window management (auto-fetched if not found)
- **nlohmann/json**: For configuration parsing (auto-fetched)
- **Random Number Generation**: For psychedelic visual triggers (C++ standard library)

## Build Instructions

### Step 1: Environment Setup

1. **Install Visual Studio 2022**
   ```powershell
   # Download from https://visualstudio.microsoft.com/
   # Select "Desktop development with C++" workload
   ```

2. **Install CMake**
   ```powershell
   # Download from https://cmake.org/download/
   # Or use chocolatey: choco install cmake
   ```

3. **Install Vulkan SDK**
   ```powershell
   # Download from https://vulkan.lunarg.com/
   # Run installer and set VULKAN_SDK environment variable
   ```

4. **Verify Installation**
   ```powershell
   # Check Visual Studio
   cl.exe
   
   # Check CMake
   cmake --version
   
   # Check Vulkan
   echo $env:VULKAN_SDK
   ```

### Step 2: Source Code Preparation

1. **Open Project Root**
   ```powershell
   # Ensure you are in the NeonGlyph project root
   cd <project-root>
   ```

2. **Initialize Submodules** (if applicable)
   ```powershell
   git submodule update --init --recursive
   ```

### Step 3: Build Process

1. **Open Visual Studio Developer Command Prompt**
   ```powershell
   # Search for "Developer Command Prompt for VS 2022"
   # Or run from Start Menu
   ```

2. **Navigate to Project Directory**
   ```powershell
   cd <project-root>
   ```

3. **Run Build Script**
   ```powershell
   # Execute the build script
   .\build.bat
   ```

4. **Alternative: Manual CMake Build**
   ```powershell
   # Create build directory
   mkdir build
   cd build
   
   # Configure
   cmake .. -G "Visual Studio 17 2022" -A x64
   
   # Build
   cmake --build . --config Release
   ```

### Step 4: Verification

1. **Check Build Output**
   ```powershell
   # Verify executable was created
   dir build\Release\NeonGlyph.exe
   dir build\Release\StoryDemo.exe
   dir build\Release\AudioTest.exe
   dir build\Release\e2e_test.exe
   
   # Check file size (should be ~2-5MB for main executable)
   ```

2. **Run Basic Test**
   ```powershell
   # Navigate to build directory
   cd build\Release
   
   # Run main executable
   .\NeonGlyph.exe --help
   
   # Test audio capture
   .\AudioTest.exe
   
   # Run end-to-end test
   .\e2e_test.exe
   ```

3. **Verify Optional Features**
   ```powershell
   # Check if ONNX Runtime is linked (AI Director)
   dumpbin /dependents NeonGlyph.exe | findstr onnxruntime
   
   # Check if FreeType is linked (Font support)
   dumpbin /dependents NeonGlyph.exe | findstr freetype
   ```

## Build Configurations

### Release Configuration
- Optimized for performance
- Minimal debug information
- Recommended for production use

### Debug Configuration
- Full debug symbols
- Assertions enabled
- Use for development and debugging

```powershell
# Debug build
cmake --build . --config Debug

# Release build
cmake --build . --config Release
```

## Advanced Build Options

### Compiler Flags
The following flags are automatically configured in CMakeLists.txt:

```cmake
# MSVC-specific flags
/W4          # Warning level 4
/permissive- # Strict conformance
/Zc:__cplusplus # Correct __cplusplus macro
/MP          # Multi-processor compilation
/O2          # Maximum optimization (Release)
/WX          # Treat warnings as errors
```

### Optional Features Configuration
```cmake
# ONNX Runtime integration (auto-detected)
find_path(ONNX_INCLUDE_DIR onnxruntime_cxx_api.h ...)
find_library(ONNX_RUNTIME_LIB NAMES onnxruntime ...)

# FreeType integration (auto-detected)
find_package(Freetype QUIET)

# GLFW integration (auto-fetched if not found)
FetchContent_Declare(glfw ...)

# JSON library (auto-fetched)
FetchContent_Declare(nlohmann_json ...)
```

### Custom Build Options
```powershell
# Build with specific toolchain
cmake .. -T v143

# Build with custom install prefix
cmake .. -DCMAKE_INSTALL_PREFIX=C:\NeonGlyph

# Build with specific C++ standard
cmake .. -DCMAKE_CXX_STANDARD=20
```

## Troubleshooting

### Common Build Errors

1. **Vulkan SDK Not Found**
   ```
   Error: Could not find Vulkan
   ```
   **Solution**: Ensure VULKAN_SDK environment variable is set
   ```powershell
   $env:VULKAN_SDK = "C:\VulkanSDK\1.3.xxx.x"
   ```

2. **CMake Version Too Old**
   ```
   Error: CMake 3.20 or higher is required
   ```
   **Solution**: Update CMake to latest version

3. **Missing Visual Studio Components**
   ```
   Error: Could not find MSVC toolset
   ```
   **Solution**: Install C++ development workload in Visual Studio

4. **Windows SDK Not Found**
   ```
   Error: Windows.h not found
   ```
   **Solution**: Install Windows 10 SDK through Visual Studio Installer

5. **ONNX Runtime Not Found**
   ```
   Warning: ONNX Runtime not found; AIConductor will be disabled
   ```
   **Solution**: Install ONNX Runtime or set ONNXRUNTIME_DIR environment variable
   ```powershell
   $env:ONNXRUNTIME_DIR = "C:\Program Files\onnxruntime"
   ```

6. **FreeType Not Found**
   ```
   Warning: FreeType not found; font atlas generation will be disabled
   ```
   **Solution**: Install FreeType development libraries

### Performance Issues

1. **Slow Build Times**
   - Enable parallel compilation: `/MP` flag
   - Use SSD for build directory
   - Close unnecessary applications

2. **Large Executable Size**
   - Strip debug symbols in Release mode
   - Enable link-time optimization: `/GL` and `/LTCG`
   - Remove unused code sections

## Build Verification Tests

### Automated Tests
```powershell
# Run unit tests
cd build
ctest --verbose

# Run specific test
.\Release\e2e_test.exe

# Test audio capture
.\Release\AudioTest.exe

# Test story demo
.\Release\StoryDemo.exe
```

### Manual Verification
1. **Audio Capture Test**
   - Play audio on system
   - Verify audio visualization appears
   - **NEW**: Check for psychedelic DMT/acid trip visuals (35-50% probability)

2. **Vulkan Initialization Test**
   - Check GPU detection
   - Verify compute shader compilation
   - **NEW**: Confirm psychedelic visual effects rendering

3. **ASCII Rendering Test**
   - Confirm ASCII output generation
   - Check frame rate performance
   - **NEW**: Verify fractal patterns and sacred geometries appear randomly

## Packaging and Distribution

### Creating Installer
```powershell
# Build installer with CPack
cd build
cpack -G NSIS
```

### Manual Packaging
```powershell
# Create distribution directory
mkdir dist\NeonGlyph

# Copy executable
copy build\Release\NeonGlyph.exe dist\NeonGlyph\

# Copy dependencies
copy %VULKAN_SDK%\Bin\vulkan-1.dll dist\NeonGlyph\

# Copy assets
xcopy /E assets dist\NeonGlyph\assets\
```

## Continuous Integration

### Build Matrix
- Windows 10 x64
- Windows 11 x64
- Visual Studio 2022
- CMake 3.20+
- Vulkan SDK latest

### Automated Checks
- Code compilation
- Unit tests execution
- Static analysis
- Performance benchmarks

## References

- [CMake Documentation](https://cmake.org/documentation/)
- [Vulkan SDK Guide](https://vulkan.lunarg.com/doc/sdk)
- [Visual Studio C++ Documentation](https://docs.microsoft.com/en-us/cpp/)
- [NeonGlyph Architecture Overview](architecture-overview.md)

## Change History

### Version 2.2.0 (2025-11-13)
- **MAJOR**: Added psychedelic visual generation build requirements
- Updated optional dependencies to include random number generation for DMT/acid trip triggers
- Enhanced manual verification steps with psychedelic visual testing
- Added consciousness-expanding visual effects to build verification
- Updated performance targets for psychedelic rendering (maintains <16ms latency)

### Version 2.1.0 (2025-11-13)
- Added optional dependencies section (ONNX Runtime, FreeType, GLFW, nlohmann/json)
- Enhanced verification steps with multiple executable tests
- Added optional features configuration documentation
- Updated troubleshooting with ONNX Runtime and FreeType issues
- Added performance optimization guidance
- Enhanced automated testing section with additional test executables

### Version 2.0.0 (2024-11-13)
- Complete rewrite following documentation style guide
- Added comprehensive troubleshooting section
- Updated build instructions for latest tools
- Added packaging and CI information

### Version 1.0.0 (2024-01-01)
- Initial build guide creation
- Basic CMake instructions
*** End of File