# NeonGlyph Branding Kit & Windows Integration

## Overview

The NeonGlyph project now includes a comprehensive branding kit with professional visual assets and deep Windows system integration. This document outlines all the new features and how to use them.

## Branding Assets

### Visual Identity

**Primary Logo**: `@` symbol with cyberpunk gradient (neon cyan to purple)
- **Main Icon**: Multi-size ICO file (16-256px) with cyberpunk @ symbol
- **Tray Icon**: 32x32px optimized for system tray display
- **Favicon**: 16x16px ultra-compact @ symbol for browser tabs
- **Splash Screen**: 512x512px full branding with subtitle

### Asset Locations

```
assets/branding/
├── neonglyph-icon.svg          # Primary application icon
├── neonglyph-tray-icon.svg     # System tray icon
├── neonglyph-favicon.svg       # Browser favicon
├── neonglyph-splash.svg        # Splash screen
└── README.md                   # Branding guidelines

resources/
├── neonglyph-icon.ico          # Multi-size Windows icon
├── neonglyph-tray-icon.ico     # System tray icon
└── app.rc                      # Windows resource definitions
```

## Windows Integration Features

### System Tray Integration

**Automatic Features**:
- System tray icon appears on application start
- Context menu with AI control options
- Double-click to toggle fullscreen
- Right-click for context menu

**Tray Menu Options**:
- Show/Hide NeonGlyph
- Start AI Show (Alt+P)
- Stop AI Show (Alt+S) 
- Take AI Break (Alt+B)
- Exit Application

### Keyboard Shortcuts

**Global Shortcuts** (work anywhere in the application):
- `F11` - Toggle fullscreen mode
- `ESC` - Exit fullscreen mode
- `Alt+P` - Start AI show (from break or cold start)
- `Alt+S` - Stop AI show gracefully
- `Alt+B` - Pause AI show (take a break)

### Window Modes

**Three Display Modes**:
1. **Windowed** - Standard window with decorations
2. **Borderless** - No window decorations, taskbar visible
3. **Fullscreen** - Complete fullscreen, no taskbar

**Mode Transitions**:
- Seamless switching between modes
- State preservation during transitions
- Automatic window placement restoration

### Windows Startup Integration

**Automatic Startup**:
- Registry integration for Windows startup
- Command-line option `--no-startup` to disable
- Automatic registration on first run

**Startup Behavior**:
- Minimized to system tray
- Quick access via tray icon
- Configurable through command line

## Command Line Arguments

```bash
NeonGlyph.exe [options]

Options:
  --fullscreen     Start in fullscreen mode
  --borderless     Start in borderless mode  
  --no-tray        Disable system tray integration
  --no-startup     Disable Windows startup registration
```

## Building and Installation

### Prerequisites

- Windows 10 or later
- Visual Studio 2019+ or compatible C++ compiler
- CMake 3.20+
- Vulkan SDK

### Build Instructions

```bash
# Generate build files
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# Build the project
cmake --build build --config Release

# Run icon generator (optional)
cd tools
build_ico_generator.bat
```

### Icon Generation

The project includes a custom ICO file generator that creates actual binary icon files:

```bash
cd tools
# Build and run the generator
build_ico_generator.bat

# Or manually compile and run
g++ -O2 generate_ico.cpp -o generate_ico.exe
generate_ico.exe
```

This generates:
- `neonglyph-icon.ico` - Multi-size main application icon
- `neonglyph-tray-icon.ico` - 32x32 system tray icon

## Configuration

### Window Settings

```json
{
  "window": {
    "fullscreen": false,
    "borderless": false,
    "disableTray": false,
    "width": 1280,
    "height": 720
  }
}
```

### Startup Settings

```json
{
  "startup": {
    "autoStart": true,
    "startMinimized": true,
    "showTrayIcon": true
  }
}
```

## Technical Implementation

### Windows Integration Architecture

**Core Components**:
- `WindowsIntegration` - Main integration class
- `NeonGlyphApp` - Application wrapper with branding
- `Window` - Enhanced with native handle access
- `Resource Management` - Icon and branding asset handling

**Key Features**:
- Native Windows API integration
- Global keyboard hook processing
- System tray notification area management
- Registry manipulation for startup
- Window procedure subclassing

### Branding Implementation

**Asset Pipeline**:
1. SVG source files with cyberpunk design
2. Custom ICO generator creates binary icon files
3. Windows resource compilation integration
4. Runtime icon loading and management

**Design Principles**:
- Minimal ASCII display focus
- Professional cyberpunk aesthetic
- Consistent branding across all assets
- Optimized for various display sizes

## Usage Examples

### Basic Usage
```bash
# Start normally
NeonGlyph.exe

# Start in fullscreen
NeonGlyph.exe --fullscreen

# Start without tray icon
NeonGlyph.exe --no-tray
```

### Advanced Usage
```bash
# Start in borderless mode, no startup registration
NeonGlyph.exe --borderless --no-startup

# Start minimized to tray
NeonGlyph.exe --no-tray=false
```

### Keyboard Control
```
While running:
- F11: Toggle fullscreen
- ESC: Exit fullscreen  
- Alt+P: Start AI visuals
- Alt+S: Stop AI visuals
- Alt+B: Pause AI visuals
```

## Troubleshooting

### Common Issues

**Icon not showing in system tray**:
- Ensure Windows Explorer is running
- Check if tray icon is hidden in system settings
- Verify icon files exist in resources directory

**Keyboard shortcuts not working**:
- Application must have focus for some shortcuts
- Check if other applications are intercepting shortcuts
- Verify Windows integration is properly initialized

**Startup registration failing**:
- Ensure application has registry write permissions
- Check if antivirus is blocking registry access
- Verify Windows version compatibility

### Debug Information

Enable debug output to troubleshoot issues:
```cpp
// In WindowsIntegration.cpp
#define DEBUG_WINDOWS_INTEGRATION
```

## Future Enhancements

### Planned Features
- Custom theming support
- Additional window modes
- Enhanced tray notifications
- Plugin architecture for branding
- Multi-monitor support improvements

### Contributing

To contribute to the branding kit:
1. Maintain cyberpunk aesthetic consistency
2. Test on multiple Windows versions
3. Follow existing code patterns
4. Update documentation for new features

## License

Branding assets and Windows integration code are part of the NeonGlyph project and follow the same licensing terms.