# NeonGlyph Branding Kit

## Overview

This comprehensive branding kit provides all the visual assets and system integration features for the NeonGlyph AI-driven ASCII visual synthesis engine.

## Brand Assets

### Logo Variations

#### Primary Logo (`logo.svg`)
- **Dimensions**: 1024x256px
- **Use**: Main application branding, splash screens
- **Colors**: Cyberpunk gradient (#0FF0FC to #FF4D67)
- **Background**: Dark (#0D0D0D)

#### Icon Logo (`neonglyph-icon.svg`)
- **Dimensions**: 256x256px
- **Use**: Application icons, system tray
- **Features**: Rounded corners, glow effect
- **Colors**: Same cyberpunk gradient

#### Tray Icon (`neonglyph-tray-icon.svg`)
- **Dimensions**: 32x32px
- **Use**: System tray notification area
- **Optimized**: Minimal detail for small sizes

#### Favicon (`neonglyph-favicon.svg`)
- **Dimensions**: 16x16px
- **Use**: Browser tabs, bookmarks
- **Ultra-compact**: Maximum recognizability at minimum size

#### Splash Screen (`neonglyph-splash.svg`)
- **Dimensions**: 512x512px
- **Use**: Application startup splash
- **Features**: Large @ symbol, subtitle text

### Color Palette

```css
/* Primary Gradient */
--neon-cyan: #0FF0FC;
--neon-magenta: #FF4D67;

/* Background Colors */
--dark-bg: #0D0D0D;
--pure-black: #000000;

/* Text Colors */
--light-text: #F2F2F2;
--medium-text: #CCCCCC;
```

## System Integration Features

### Window Management

#### Fullscreen Controls
- **F11**: Toggle fullscreen mode
- **ESC**: Exit fullscreen mode
- **Double-click**: Toggle fullscreen (window title bar)

#### Window Modes
- **Windowed**: Standard window with decorations
- **Borderless**: No window decorations, taskbar visible
- **Fullscreen**: Complete screen takeover

### AI Control Shortcuts

#### Global Hotkeys (Alt combinations)
- **Alt+B**: Take AI break (pause AI processing)
- **Alt+S**: Stop AI show (completely stop AI)
- **Alt+P**: Start AI show (resume/start AI processing)

### System Tray Integration

#### Tray Icon States
- **Active**: Normal gradient colors
- **Break**: Dimmed colors with pause overlay
- **Stopped**: Grayscale colors
- **Error**: Red overlay

#### Tray Menu Options
- **Show/Hide**: Toggle application visibility
- **Start AI Show**: Begin AI processing
- **Stop AI Show**: End AI processing
- **Take AI Break**: Pause AI temporarily
- **Exit**: Graceful application shutdown

### Windows Startup Integration

#### Auto-start Configuration
- Registry entry: `HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run`
- Value name: `NeonGlyph`
- Value data: Full executable path

#### Startup Options
- `--no-startup`: Disable auto-start registration
- `--start-minimized`: Begin minimized to tray
- `--minimize-to-tray`: Minimize to tray instead of taskbar

## Implementation Guide

### Building with Branding

The CMake build system automatically includes:
- Resource compilation (`resources/app.rc`)
- Icon embedding (multiple sizes)
- Version information
- Windows integration libraries

### Command Line Arguments

```bash
NeonGlyph.exe [options]

Options:
  --fullscreen        Start in fullscreen mode
  --borderless        Start in borderless mode
  --no-tray           Disable system tray icon
  --no-startup        Disable startup registration
  --help              Show help message
```

### Configuration File Integration

Add to your `config.json`:

```json
{
  "window": {
    "fullscreen": false,
    "borderless": false,
    "disableTray": false
  },
  "startup": {
    "autoStart": true,
    "minimizeToTray": true,
    "startMinimized": false
  }
}
```

### Icon Conversion Process

The branding kit includes SVG source files that can be converted to Windows ICO format:

1. **Install Dependencies**:
   - Inkscape (for SVG to PNG conversion)
   - ImageMagick (for PNG to ICO conversion)

2. **Run Conversion Script**:
   ```powershell
   cd assets/branding
   .\convert-icons.ps1
   ```

3. **Manual Conversion Alternative**:
   - Export SVG to PNG at multiple sizes (16, 24, 32, 48, 64, 128, 256px)
   - Combine PNG files into single ICO using icon editing software

### Customization Guidelines

#### Logo Modifications
- Maintain the @ symbol as the central element
- Preserve the cyberpunk gradient colors
- Keep dark background for contrast
- Ensure readability at all sizes

#### Color Variations
- **Performance Mode**: Bright cyan (#00FFFF) and magenta (#FF00FF)
- **Break Mode**: Dimmed cyan (#008080) and magenta (#800080)
- **Error Mode**: Red (#FF0000) overlay
- **Success Mode**: Green (#00FF00) overlay

#### Animation Considerations
- Use subtle glow pulsing for active state
- Implement smooth transitions between modes
- Avoid rapid flashing (epilepsy safety)
- Maintain 60+ FPS performance

## Usage Examples

### Basic Startup
```cpp
// Initialize with default branding
NeonGlyphApp app;
Config config;
app.Initialize(config);
app.Run();
```

### Custom Branding
```cpp
// Load custom configuration
ConfigManager configManager;
Config config;
configManager.Load("custom_config.json", config);

// Override specific settings
config.window.fullscreen = true;
config.startup.autoStart = true;

NeonGlyphApp app;
app.Initialize(config);
app.Run();
```

### Programmatic Control
```cpp
// Access Windows integration
auto* integration = app.GetWindowsIntegration();

// Control AI state
integration->StartAIShow();
integration->TakeAIBreak();
integration->StopAIShow();

// Control window mode
integration->SetWindowMode(WindowMode::Fullscreen);
integration->ToggleFullscreen();
```

## Troubleshooting

### Icon Not Displaying
1. Verify ICO files exist in `assets/branding/`
2. Check resource compilation in build output
3. Ensure Windows icon cache is refreshed

### System Tray Issues
1. Check Windows notification area settings
2. Verify tray icon creation in debug output
3. Ensure proper Windows message handling

### Startup Registration Problems
1. Check registry permissions
2. Verify executable path in registry
3. Test with different user accounts

### Keyboard Shortcuts Not Working
1. Verify no other applications intercept keys
2. Check Windows accessibility settings
3. Ensure application has focus

## Performance Optimization

### Icon Memory Usage
- Use appropriate icon sizes for each context
- Implement lazy loading for large icons
- Cache frequently used icons

### System Tray Efficiency
- Minimize tray updates to reduce CPU usage
- Use efficient message handling
- Implement proper cleanup on exit

### Window Mode Transitions
- Cache window placement information
- Minimize mode change delays
- Maintain rendering performance during transitions

This branding kit provides a complete visual identity and user experience framework for the NeonGlyph application, ensuring professional presentation and intuitive user interaction.