# NeonGlyph v3.1.0 Release Notes

**Release Date:** 2025-11-17
**Release Type:** Critical Bug Fix + Stability Release

---

## Executive Summary

This release addresses **critical stability issues** that caused the application to crash after startup. The primary fix resolves an out-of-bounds array access in the Vulkan semaphore synchronization code. Additionally, this release enables windowed rendering mode and removes hardcoded dependency paths for improved portability.

---

## Critical Bug Fixes

### 1. **Fixed Out-of-Bounds Semaphore Array Access** (CRITICAL)
- **Issue:** Application crashed when rendering frames with triple-buffered swapchain
- **Root Cause:** `m_renderFinishedSemaphores[m_currentImageIndex]` accessed array index 2 when array size was only 2 (`MAX_FRAMES_IN_FLIGHT`)
- **Files:** `src/VulkanContext.cpp` lines 1066, 1076
- **Impact:** 100% crash rate after a few rendered frames on systems with 3+ swapchain images

### 2. **Enabled Windowed Rendering Mode** (CRITICAL)
- **Issue:** Application compiled in "console-only" mode despite GLFW being linked
- **Root Cause:** `NEONGLYPH_HAVE_GLFW=0` set in CMakeLists.txt
- **Files:** `CMakeLists.txt` line 141
- **Impact:** No window created, no visual output

### 3. **Fixed Duplicate Error Code Values** (HIGH)
- **Issue:** `AlreadyInitialized` and `FileNotFound` both had value 8
- **Root Cause:** Copy-paste error in enum definition
- **Files:** `include/NeonGlyph.h` lines 94-95
- **Impact:** Incorrect error handling and debugging

### 4. **Fixed Missing Staging Buffer Creation** (CRITICAL)
- **Issue:** Black screen despite window and swapchain being created correctly
- **Root Cause:** `CreateFrameResources()` was never called after swapchain creation, so staging buffer remained `VK_NULL_HANDLE`
- **Files:** `src/Application.cpp` lines 234-239 (added)
- **Impact:** `UpdateFramePixels()` returned `InitializationFailed` silently, no pixels ever copied to swapchain, only clear color displayed

### 5. **Fixed Command Buffer Timing for Pixel Copy** (CRITICAL)
- **Issue:** Black screen persisted even with staging buffer created
- **Root Cause:** Copy command was recorded in `BeginFrame()` BEFORE `Render()` updated the staging buffer, so stale/empty data was copied
- **Files:** `src/VulkanContext.cpp` lines 989-1086 - Moved copy operation from `BeginFrame()` to `EndFrame()`
- **Impact:** Now command recording happens in correct order: clear → (Render updates buffer) → copy → present

---

## Build System Improvements

### 1. **Removed Hardcoded Vulkan SDK Paths**
- **Before:** `"C:/VulkanSDK/1.4.328.1/Lib/vulkan-1.lib"` hardcoded
- **After:** Uses `${Vulkan_LIBRARIES}` from CMake `find_package(Vulkan)`
- **Files:** `CMakeLists.txt` lines 117-121
- **Benefit:** Portable across different SDK versions and installations

### 2. **Platform-Specific Library Linking**
- **Before:** Windows libraries linked unconditionally
- **After:** Wrapped in `if(WIN32)` conditionals
- **Files:** `CMakeLists.txt` lines 123-137
- **Benefit:** Improved cross-platform compatibility

### 3. **Dynamic Vulkan SDK Discovery**
- **Before:** Fixed list of SDK version paths
- **After:** Scans VulkanSDK directories for any version
- **Files:** `src/VulkanContext.cpp` lines 765-786, `src/ComputePipelines.cpp` lines 458-479
- **Benefit:** Works with any Vulkan SDK version

### 4. **Platform-Specific Font Discovery**
- **Before:** Hardcoded `C:/Windows/Fonts/` path
- **After:** Uses `WINDIR` environment variable with fallback paths
- **Files:** `src/ASCIIConverter.cpp` lines 294-324
- **Benefit:** Works on different Windows installations and locales

---

## Code Quality Improvements

### 1. **Expanded Error Code Enumeration**
- Added `FileNotFound = 14` (unique value)
- Added `ConfigurationError = 15`
- Added `ResourceBusy = 16`
- **Files:** `include/NeonGlyph.h` lines 84-103
- **Benefit:** Better error diagnostics and handling

### 2. **Improved Vulkan Initialization Validation**
- Added bounds checking for swapchain image indices
- Improved error messages for debugging
- **Files:** `src/VulkanContext.cpp` throughout

---

## Testing and Verification

### Automated Tests
- All existing unit tests pass
- Configuration validation tests pass
- Vulkan resource stress tests pass

### Manual Verification Steps
1. Window creates and displays successfully
2. Swapchain cycles through all images without crash
3. Frame synchronization operates correctly
4. Window resizing/minimizing handled properly
5. Headless mode fallback works

---

## Migration Guide

### No Breaking Changes
This release is fully backward compatible. Existing configurations and usage patterns will continue to work.

### Recommended Actions
1. **Update build system** - Clean rebuild recommended
   ```bash
   rm -rf build64/
   mkdir build64 && cd build64
   cmake .. -G "Visual Studio 17 2022" -A x64
   cmake --build . --config Release
   ```

2. **Set VULKAN_SDK environment variable** (optional but recommended)
   ```powershell
   $env:VULKAN_SDK = "C:\VulkanSDK\YOUR_VERSION"
   ```

---

## Performance

No performance regressions. Frame timing remains consistent at target FPS (144 FPS default, configurable).

---

## Known Limitations

1. **Windows-only** - Audio capture uses WASAPI (Windows Audio Session API)
2. **Vulkan required** - GPU must support Vulkan 1.0+
3. **Visual Studio 2022 recommended** - Requires C++20 support

---

## File Changes Summary

| File | Changes |
|------|---------|
| `CMakeLists.txt` | Version bump to 3.1.0, enabled GLFW, fixed library linking |
| `src/VulkanContext.cpp` | Fixed semaphore indexing, improved SDK discovery |
| `src/Application.cpp` | Added missing CreateFrameResources() call for staging buffer creation |
| `src/ComputePipelines.cpp` | Improved SDK discovery |
| `src/ASCIIConverter.cpp` | Platform-specific font paths |
| `include/NeonGlyph.h` | Fixed duplicate error codes, added new codes |
| `RELEASE_BUILD_GUIDE.md` | New comprehensive build guide |
| `CRASH_DIAGNOSIS_POST_PR.md` | Detailed crash analysis |
| `STARTUP_RENDERING_DIAGNOSIS.md` | Initial diagnosis report |

---

## Acknowledgments

Special thanks to the community for reporting the crash issues and providing detailed logs.

---

## Download

- **Windows x64 Standalone**: `NeonGlyph-v3.1.0-windows-x64.zip`
- **Source Code**: GitHub repository

---

## Upgrade Command

```bash
git pull origin main
git checkout v3.1.0
```

---

**Full Changelog**: https://github.com/Snapwave333/ASCII/compare/v3.0.1...v3.1.0

---

## Next Release Preview (v3.2.0)

Planned features:
- AI Conductor integration
- Enhanced performance monitoring
- NDI/Spout output improvements
- Additional color palettes
