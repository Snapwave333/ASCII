# NeonGlyph v3.1.0 Release - Build and Test Guide

## Release Summary

This release fixes critical bugs including:
- **CRITICAL**: Out-of-bounds semaphore array access causing crashes (VulkanContext.cpp:1066, 1076)
- **CRITICAL**: GLFW windowed rendering disabled by default (CMakeLists.txt:136)
- **HIGH**: Hardcoded Vulkan SDK paths preventing portable builds
- **HIGH**: Duplicate error codes causing undefined behavior
- **MEDIUM**: Hardcoded font paths preventing cross-platform usage

## Prerequisites

### Required Software
- **Windows 10/11** (64-bit)
- **Visual Studio 2022** (Community or higher) with C++ Desktop Development workload
- **CMake 3.20+** (included with VS2022 or download separately)
- **Vulkan SDK 1.3+** (download from https://vulkan.lunarg.com/)
- **Git** (for cloning)

### System Requirements
- GPU with Vulkan 1.0+ support (NVIDIA, AMD, or Intel)
- 8GB RAM minimum, 16GB recommended
- 2GB disk space for build artifacts

## Build Instructions

### Option 1: Quick Build (Recommended)

```powershell
# Open x64 Native Tools Command Prompt for VS 2022
cd C:\path\to\NeonGlyph

# Create build directory
mkdir build64
cd build64

# Configure (Release build)
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --config Release --parallel

# Output executable
# build64\Release\NeonGlyph.exe
```

### Option 2: Visual Studio IDE

1. Open Visual Studio 2022
2. File → Open → CMake → Select `CMakeLists.txt`
3. Select `x64-Release` configuration
4. Build → Build All (Ctrl+Shift+B)
5. Executable at `out\build\x64-Release\NeonGlyph.exe`

### Option 3: Using build.bat

```batch
# From project root directory
build.bat
# Follow prompts, select Release configuration
```

## Verification Tests

### Test 1: Basic Startup
```powershell
cd build64\Release
.\NeonGlyph.exe
```
**Expected**: Window opens with black background, console shows:
- `[Application] Initializing optimized Vulkan context...`
- `[Application] Creating Vulkan instance...`
- `Vulkan instance created successfully`
- No crash after 10+ seconds

### Test 2: Swapchain Frame Cycling
```powershell
.\NeonGlyph.exe
# Let run for 30 seconds
```
**Expected**: Cycles through all swapchain images (2-3) without crash. Verifies semaphore index fix.

### Test 3: Window Interactions
```powershell
.\NeonGlyph.exe
# Try: Minimize, Maximize, Resize, Move window
```
**Expected**: Stable operation, no crashes during window state changes.

### Test 4: Configuration Loading
```powershell
.\NeonGlyph.exe --config config\default.json
```
**Expected**: Loads configuration successfully, applies settings.

### Test 5: Headless Mode Fallback
```powershell
.\NeonGlyph.exe --headless
```
**Expected**: Runs without creating window, console-only mode.

### Test 6: Performance Metrics
```powershell
$env:NG_METRICS_PATH="metrics.log"
.\NeonGlyph.exe
```
**Expected**: Metrics logged to file, frame times < 16ms (60 FPS).

## Automated Test Suite

```powershell
# Enable tests in CMake
cmake .. -DNEONGLYPH_BUILD_TESTS=ON
cmake --build . --config Release

# Run all tests
ctest -C Release --output-on-failure
```

### Individual Tests
- `AudioTest.exe` - Audio subsystem
- `e2e_test.exe` - End-to-end integration
- `config_validation_tests.exe` - Configuration parsing
- `vulkan_resource_stress_test.exe` - GPU resource management

## Creating Standalone Release Package

### Package Contents

```
NeonGlyph-v3.1.0-windows-x64/
├── NeonGlyph.exe           # Main executable
├── config/
│   ├── default.json        # Default configuration
│   ├── palettes.json       # Color palettes
│   ├── charsets.json       # ASCII character sets
│   └── themes/             # UI themes
├── shaders/
│   ├── ascii_convert.comp  # Compute shaders
│   └── beat_detection.comp
├── docs/
│   ├── README.md           # User guide
│   ├── CHANGELOG.md        # Version history
│   └── TROUBLESHOOTING.md  # Common issues
├── VCRUNTIME140.dll        # Visual C++ Runtime (if not installed)
├── vulkan-1.dll            # Vulkan loader (optional)
└── LICENSE
```

### Packaging Script

```powershell
# Create release directory
$version = "3.1.0"
$releaseDir = "NeonGlyph-v$version-windows-x64"

New-Item -ItemType Directory -Path $releaseDir

# Copy executable
Copy-Item "build64\Release\NeonGlyph.exe" "$releaseDir\"

# Copy configuration
Copy-Item -Recurse "config" "$releaseDir\config"

# Copy shaders
Copy-Item -Recurse "shaders" "$releaseDir\shaders"

# Copy documentation
New-Item -ItemType Directory -Path "$releaseDir\docs"
Copy-Item "README.md" "$releaseDir\docs\"
Copy-Item "CHANGELOG.md" "$releaseDir\docs\"

# Copy license
Copy-Item "LICENSE" "$releaseDir\"

# Create archive
Compress-Archive -Path $releaseDir -DestinationPath "NeonGlyph-v$version-windows-x64.zip"

Write-Host "Release package created: NeonGlyph-v$version-windows-x64.zip"
```

## Known Issues and Workarounds

### Issue: "Vulkan SDK not found"
**Solution**: Set VULKAN_SDK environment variable:
```powershell
$env:VULKAN_SDK = "C:\VulkanSDK\1.3.xxx.x"
```

### Issue: "GLFW not found"
**Solution**: CMake will automatically fetch GLFW 3.3.9 via FetchContent.

### Issue: "Visual C++ Runtime not found"
**Solution**: Install Visual C++ Redistributable 2019/2022 from Microsoft.

### Issue: App crashes immediately
**Solution**:
1. Verify GPU drivers are up to date
2. Check Vulkan SDK installation
3. Run with `--headless` to test core functionality

## Performance Tuning

Edit `config/default.json`:

```json
{
  "window": {
    "width": 1920,
    "height": 1080,
    "targetFPS": 60  // Lower for older GPUs
  },
  "ascii": {
    "gridWidth": 120,  // Reduce for better performance
    "gridHeight": 48
  },
  "render": {
    "vsync": true,     // Enable for smooth rendering
    "blackStartup": true
  }
}
```

## Submitting to GitHub Releases

### Manual Release

1. Go to GitHub repository → Releases → Draft new release
2. Create tag: `v3.1.0`
3. Upload `NeonGlyph-v3.1.0-windows-x64.zip`
4. Add release notes from CHANGELOG.md
5. Publish release

### CLI Release (gh tool required)

```bash
gh release create v3.1.0 \
  --title "NeonGlyph v3.1.0 - Critical Bug Fixes" \
  --notes-file RELEASE_NOTES.md \
  NeonGlyph-v3.1.0-windows-x64.zip
```

## Post-Release Checklist

- [ ] Build completes without errors
- [ ] All verification tests pass
- [ ] No crashes during 10-minute stress test
- [ ] Configuration files validated
- [ ] Documentation updated
- [ ] CHANGELOG.md reflects all changes
- [ ] Version number bumped in CMakeLists.txt
- [ ] Release package created and tested on clean system
- [ ] GitHub release tagged and published
- [ ] Release notes posted

## Contact and Support

- GitHub Issues: https://github.com/Snapwave333/ASCII/issues
- Documentation: `/docs` directory
- Discord: (if available)

---

**Build Date**: 2025-11-17
**Version**: 3.1.0
**Platform**: Windows x64
**Compiler**: MSVC 19.3x (Visual Studio 2022)
