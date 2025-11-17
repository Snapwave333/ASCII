# NeonGlyph Branding Kit - Enhanced Assets

## Overview
This comprehensive branding kit provides all visual assets for the NeonGlyph project with cyberpunk aesthetic and @ symbol branding.

## Asset Collection

### Primary Icons
- `neonglyph-icon-enhanced.svg` - Main application icon (256x256px)
- `neonglyph-tray-icon-enhanced.svg` - System tray icon (32x32px)
- `neonglyph-favicon-enhanced.svg` - Browser favicon (16x16px)

### Standard Assets
- `neonglyph-icon.svg` - Standard main icon
- `neonglyph-tray-icon.svg` - Standard tray icon
- `neonglyph-favicon.svg` - Standard favicon
- `neonglyph-splash.svg` - Splash screen asset

### Multi-Size Icons
- `neonglyph-icon-24.svg` - 24x24px variant
- `neonglyph-icon-32.svg` - 32x32px variant
- `neonglyph-icon-48.svg` - 48x48px variant
- `neonglyph-icon-64.svg` - 64x64px variant

## Design Specifications

### Color Palette
- **Primary Gradient**: #0FF0FC (Cyan) → #FF4D67 (Neon Pink)
- **Background**: #0D0D0D (Deep Black)
- **Glow Effects**: Gaussian blur with 20% opacity overlay

### Typography
- **Font**: Consolas, monospace
- **Symbol**: @ (At symbol - represents AI/ASCII theme)
- **Weight**: Bold, 144pt for main icon

### Visual Effects
- **Glow Filter**: 3-4px Gaussian blur with merge
- **Outer Glow**: 8px blur with color matrix enhancement
- **Concentric Circles**: Decorative elements with 30-50% opacity
- **Rounded Corners**: 48px radius for main icon, 6px for tray

## Usage Guidelines

### Application Icons
Use enhanced versions for main application windows and high-DPI displays:
```cpp
// Windows resource reference
IDI_ICON1 ICON "assets/branding/neonglyph-icon-enhanced.ico"
```

### System Tray
Tray icon should use enhanced version for better visibility:
```cpp
// System tray creation
NOTIFYICONDATA nid = {0};
nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TRAY_ICON));
```

### Favicon
Browser favicon uses 16x16px enhanced version for crisp display.

### Window Integration
Icons should be applied to:
- Main window title bar
- Taskbar thumbnails
- Alt+Tab switcher
- File associations

## Windows Integration

### ICO File Generation
All SVG assets should be converted to ICO format for Windows integration:
- Multi-size ICO files (16, 24, 32, 48, 64, 128, 256px)
- Proper BMP format within ICO container
- Alpha channel support for transparency

### Registry Integration
Icons are registered for:
- File type associations
- Start menu shortcuts
- Desktop shortcuts
- Windows Explorer integration

## Keyboard Shortcuts
The branding supports the following shortcuts:
- **F11**: Toggle fullscreen
- **ESC**: Exit fullscreen
- **Alt+B**: AI break/pause
- **Alt+S**: Stop show gracefully
- **Alt+P**: Start/pause AI visuals
- **Double-click**: Toggle windowed/borderless

## Pure Visual Mode
Enhanced assets support pure visual mode where:
- No HUD information displayed to viewers
- All AI data processing remains internal
- Clean ASCII visual experience
- System tray integration for control

## Asset Generation
Use the provided PowerShell script `convert-icons.ps1` for batch ICO conversion, or the C++ icon generator for programmatic creation.