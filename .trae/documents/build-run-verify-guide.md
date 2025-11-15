# NeonGlyph Build, Run, and Verification Guide

**Document Type**: Technical Guide
**Version**: 2.0.0
**Last Updated**: 2024-11-13
**Status**: Approved
**Author**: NeonGlyph Development Team

## Overview

This guide provides comprehensive instructions for building, running, and verifying the NeonGlyph Vulkan ASCII VJ application, including preflight checks, build procedures, runtime verification, and diagnostics.

## Preflight Checks

### System Requirements Verification

1. **Visual Studio 2022 Verification**
   ```powershell
   # Check Visual Studio installation
   vswhere -latest -property installationPath
   
   # Verify MSVC toolchain
   cl.exe
   ```

2. **Vulkan SDK Verification**
   ```powershell
   # Check Vulkan SDK environment variable
   echo $env:VULKAN_SDK
   
   # Verify Vulkan runtime
   vulkaninfoSDK
   
   # Check vulkan-1.dll in PATH
   where vulkan-1.dll
   ```

3. **GPU Driver Verification**
   ```powershell
   # Check GPU driver version
   wmic path win32_VideoController get name, driverversion
   
   # Update to latest drivers from manufacturer
   ```

### Development Environment Setup

1. **Visual Studio Developer Command Prompt**
   ```powershell
   # Open from Start Menu: "Developer Command Prompt for VS 2022"
   # Or use shortcut: Win + X → "Developer Command Prompt"
   ```

2. **Environment Variables**
   ```powershell
   # Set Vulkan SDK path if not automatically configured
   $env:VULKAN_SDK = "C:\VulkanSDK\1.3.xxx.x"
   
   # Add to PATH if necessary
   $env:PATH += ";$env:VULKAN_SDK\Bin"
   ```

## Build and Launch

### Automated Build Process

1. **Navigate to Project Root**
   ```powershell
   cd C:\Users\YourUsername\Documents\NeonGlyph
   ```

2. **Execute Build Script**
   ```powershell
   # Run the build script
   .\build.bat
   ```

3. **Build Output Verification**
   ```powershell
   # Check for successful build artifacts
   dir build\Release\NeonGlyph.exe
   dir build\Release\*.dll
   ```

### Manual CMake Build (Alternative)

1. **Create Build Directory**
   ```powershell
   mkdir build
   cd build
   ```

2. **Configure CMake**
   ```powershell
   cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
   ```

3. **Build Project**
   ```powershell
   cmake --build . --config Release --parallel
   ```

### Application Launch

1. **Start Audio Source**
   ```powershell
   # Ensure system audio is playing (music, video, etc.)
   # WASAPI loopback will capture system audio
   ```

2. **Launch Application**
   ```powershell
   # Navigate to build output
   cd build\Release
   
   # Launch NeonGlyph
   .\NeonGlyph.exe
   
   # Or with command line options
   .\NeonGlyph.exe --config custom_config.json --fullscreen
   ```

## Runtime Verification

### Audio Pipeline Verification

1. **Audio Capture Verification**
   ```powershell
   # Check application logs for audio initialization
   # Look for "WASAPI initialized successfully"
   # Verify audio device enumeration
   ```

2. **Audio Feature Extraction**
   ```powershell
   # Monitor audio metrics in real-time
   # Expected: non-zero volume levels, BPM detection, spectrum data
   # Check for audio buffer underruns
   ```

3. **Audio-to-Visual Pipeline**
   ```powershell
   # Verify audio features are driving visual changes
   # Test with different music genres and tempos
   # Confirm visual responsiveness to audio changes
   ```

### AI Director Verification

1. **Model Loading Verification**
   ```powershell
   # Check ONNX model loading status
   # Verify DirectML execution provider
   # Confirm model input/output shapes
   ```

2. **Scene Transition Verification**
   ```powershell
   # Monitor scene changes based on music sections
   # Verify verse/chorus/bridge detection
   # Check pacing and shot transition timing
   ```

3. **Command Generation Verification**
   ```powershell
   # Verify DirectorCommand generation
   # Check command parameters (intensity, style, palette)
   # Validate command timing and duration
   ```

### Renderer Verification

1. **Vulkan Initialization**
   ```powershell
   # Check Vulkan instance creation
   # Verify device selection and capabilities
   # Confirm compute shader compilation
   ```

2. **ASCII Rendering Verification**
   ```powershell
   # Verify ASCII grid rendering
   # Check character density and distribution
   # Confirm color palette application
   ```

3. **Performance Metrics**
   ```powershell
   # Monitor frame rate (target: 60+ FPS)
   # Check frame time consistency
   # Verify draw call count (<800 per frame)
   ```

### Frame Rate and Performance Verification

1. **Performance Monitoring**
   ```powershell
   # Enable performance overlay
   # Monitor CPU and GPU usage
   # Check memory consumption
   ```

2. **Frame Consistency**
   ```powershell
   # Verify consistent frame timing
   # Check for frame drops or stuttering
   # Monitor audio-visual synchronization
   ```

3. **Resource Usage**
   ```powershell
   # Monitor system resource consumption
   # Check for memory leaks
   # Verify GPU memory usage
   ```

## Diagnostics and Troubleshooting

### Common Issues and Solutions

#### Issue: Black Screen or No Visual Output

**Symptoms**: Application launches but no visual output

**Diagnostic Steps**:
1. Check Vulkan surface creation
2. Verify swapchain initialization
3. Confirm render pass setup
4. Validate framebuffer creation

**Solutions**:
```powershell
# Enable Vulkan validation layers
$env:VK_INSTANCE_LAYERS = "VK_LAYER_KHRONOS_validation"

# Check GPU compatibility
vulkaninfoSDK | findstr "GPU"

# Verify surface capabilities
# Check window creation and surface properties
```

#### Issue: Audio Capture Not Working

**Symptoms**: No audio visualization or feature extraction

**Diagnostic Steps**:
1. Verify WASAPI device enumeration
2. Check audio format compatibility
3. Confirm loopback recording permissions
4. Validate audio buffer management

**Solutions**:
```powershell
# Check default audio device
Get-AudioDevice -List | Where-Object {$_.Default -eq $true}

# Test with different audio sources
# Verify Windows audio service is running
Get-Service -Name "Audiosrv"

# Check audio endpoint permissions
# Ensure application has microphone access
```

#### Issue: ASCII Rendering Problems

**Symptoms**: Incorrect character mapping or visual artifacts

**Diagnostic Steps**:
1. Verify instance buffer layout
2. Check character atlas texture
3. Validate compute shader execution
4. Confirm descriptor set bindings

**Solutions**:
```powershell
# Enable detailed logging
# Check instance buffer stride and offsets
# Verify glyph index mapping to atlas tiles
# Validate compute pipeline execution
```

#### Issue: Performance Problems

**Symptoms**: Low frame rate or high CPU/GPU usage

**Diagnostic Steps**:
1. Profile hot paths and bottlenecks
2. Check for unnecessary allocations
3. Verify parallel processing utilization
4. Monitor GPU command submission

**Solutions**:
```powershell
# Enable performance profiling
# Optimize compute shader workgroup sizes
# Reduce ASCII grid resolution if necessary
# Check for CPU-GPU synchronization issues
```

### Advanced Diagnostics

#### Vulkan Validation Layers

```powershell
# Enable comprehensive validation
$env:VK_INSTANCE_LAYERS = "VK_LAYER_KHRONOS_validation"
$env:VK_DEVICE_LAYERS = "VK_LAYER_KHRONOS_validation"

# Set validation features
$env:VK_VALIDATION_FEATURES = "VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION"
```

#### Performance Profiling

```powershell
# Use GPUView for detailed GPU profiling
# Enable ETW events for graphics analysis
# Use Visual Studio graphics debugger
# Monitor GPU memory allocation patterns
```

#### Memory Analysis

```powershell
# Use Application Verifier for memory debugging
# Enable CRT debug heap
# Check for memory leaks with Valgrind (Linux)
# Monitor allocation patterns and hotspots
```

## Acceptance Criteria

### Functional Requirements

- [ ] Audio capture and analysis pipeline operational
- [ ] AI Director generating appropriate scene commands
- [ ] ASCII rendering with correct character mapping
- [ ] Vulkan renderer achieving target frame rate
- [ ] Audio-visual synchronization within 16ms

### Performance Requirements

- [ ] Frame rate: 60+ FPS at target resolution
- [ ] CPU usage: <5% on idle, <25% during operation
- [ ] GPU usage: <50% for rendering tasks
- [ ] Memory usage: <2GB baseline consumption
- [ ] Draw calls: <800 per frame

### Quality Requirements

- [ ] No visual artifacts or rendering errors
- [ ] Smooth audio-visual synchronization
- [ ] Responsive scene transitions
- [ ] Stable performance over extended periods
- [ ] Proper cleanup and resource management

## Contingency Procedures

### Emergency Fallback Modes

1. **Console Output Mode**
   ```powershell
   # Enable console-only output
   .\NeonGlyph.exe --console-mode
   ```

2. **Reduced Performance Mode**
   ```powershell
   # Lower resolution and frame rate
   .\NeonGlyph.exe --resolution 1280x720 --fps 30
   ```

3. **Software Rendering Mode**
   ```powershell
   # Use CPU-based rendering (fallback)
   .\NeonGlyph.exe --software-rendering
   ```

### Recovery Procedures

1. **Application Crash Recovery**
   ```powershell
   # Automatic crash detection and restart
   # Preserve last known good configuration
   # Generate crash dump for analysis
   ```

2. **Configuration Reset**
   ```powershell
   # Reset to default configuration
   # Clear corrupted settings
   # Restore factory defaults
   ```

3. **System Recovery**
   ```powershell
   # Clean up orphaned resources
   # Reset audio/video subsystem
   # Restart Windows audio service
   ```

## Next Steps and Enhancements

### Beat-Synced Motion

- Implement phase-locked loop for beat detection
- Add motion synchronization to audio phase
- Create beat-reactive visual effects

### Advanced Color Mapping

- Map mise-en-scene palette to fragment shader
- Implement dynamic color palette generation
- Add color harmony and contrast optimization

### Output Integration

- Plan Spout integration for live streaming
- Implement NDI broadcasting capabilities
- Add support for multiple output formats

### Performance Optimization

- Optimize compute shader algorithms
- Implement adaptive quality scaling
- Add multi-threading optimizations

## References

- [NeonGlyph Build Guide](build-guide.md)
- [NeonGlyph Architecture Overview](architecture-overview.md)
- [NeonGlyph Configuration Guide](configuration-guide.md)
- [Vulkan SDK Documentation](https://vulkan.lunarg.com/doc/sdk)
- [WASAPI Documentation](https://docs.microsoft.com/en-us/windows/win32/coreaudio/wasapi)
- [ONNX Runtime Documentation](https://onnxruntime.ai/docs/)

## Change History

### Version 2.0.0 (2024-11-13)
- Complete rewrite following documentation style guide
- Added comprehensive verification procedures
- Enhanced troubleshooting section with detailed diagnostics
- Added acceptance criteria and contingency procedures

### Version 1.0.0 (2024-01-01)
- Initial build and verification guide
- Basic build instructions and runtime checks