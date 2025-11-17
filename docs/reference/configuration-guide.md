# NeonGlyph Configuration Guide

**Document Type**: Configuration Guide
**Version**: 2.1.0
**Last Updated**: 2025-11-13
**Status**: Approved
**Author**: NeonGlyph Development Team

## Overview

This document provides comprehensive configuration options for the NeonGlyph AI-driven ASCII visual synthesis engine, including theme customization, performance tuning, and system integration settings.

## Configuration File Structure

### Main Configuration File

**Location**: `config/default.json`

```json
{
  "application": {
    "name": "NeonGlyph",
    "version": "1.0.0",
    "log_level": "info",
    "auto_start": false,
    "hot_reload": {
      "enabled": true,
      "watch_files": ["config/themes/*.json"],
      "check_interval": 5000
    }
  },
  "window": {
    "width": 1920,
    "height": 1080,
    "fps": 144,
    "fullscreen": false,
    "vsync": false,
    "borderless": true,
    "always_on_top": false
  },
  "audio": {
    "sample_rate": 48000,
    "buffer_size": 1024,
    "device": "default",
    "loopback": true,
    "channels": 2,
    "bit_depth": 16
  },
  "renderer": {
    "gpu_index": 0,
    "msaa_samples": 4,
    "compute_threads": 8,
    "memory_limit_mb": 2048,
    "validation_layers": false
  }
}
```

## Theme Configuration

### Theme File Structure

**Location**: `config/themes/[theme-name].json`

```json
{
  "theme": {
    "name": "cyberpunk_neon",
    "version": "1.0.0",
    "metadata": {
      "author": "NeonGlyph Team",
      "description": "Cyberpunk-inspired neon theme",
      "tags": ["cyberpunk", "neon", "dark"]
    },
    "colors": {
      "primary": "#0FF0FC",
      "secondary": "#FF4D67",
      "accent": "#FFE066",
      "background": "#0D0D0D",
      "foreground": "#F2F2F2",
      "success": "#35FF69",
      "warning": "#FF8C00",
      "error": "#FF0000"
    },
    "ascii": {
      "characters": " .:-=+*#%@",
      "brightness": 0.85,
      "contrast": 1.2,
      "invert": false,
      "colorize": true
    },
    "typography": {
      "font_family": "Consolas",
      "font_size": 12,
      "line_height": 1.2,
      "letter_spacing": 1.0
    }
  }
}
```

### Predefined Themes

#### Dark Theme (`config/themes/dark.json`)
```json
{
  "theme": {
    "name": "dark",
    "colors": {
      "primary": "#FFFFFF",
      "secondary": "#CCCCCC",
      "background": "#000000",
      "foreground": "#FFFFFF"
    },
    "ascii": {
      "characters": " .:-=+*#%@",
      "brightness": 0.9,
      "contrast": 1.0
    }
  }
}
```

#### Light Theme (`config/themes/light.json`)
```json
{
  "theme": {
    "name": "light",
    "colors": {
      "primary": "#000000",
      "secondary": "#333333",
      "background": "#FFFFFF",
      "foreground": "#000000"
    },
    "ascii": {
      "characters": " .:-=+*#%@",
      "brightness": 0.7,
      "contrast": 0.8
    }
  }
}
```

#### Neon Theme (`config/themes/neon.json`)
```json
{
  "theme": {
    "name": "neon",
    "colors": {
      "primary": "#0FF0FC",
      "secondary": "#FF4D67",
      "accent": "#FFE066",
      "background": "#0D0D0D",
      "foreground": "#F2F2F2"
    },
    "ascii": {
      "characters": " ░▒▓█",
      "brightness": 0.85,
      "contrast": 1.3,
      "colorize": true
    }
  }
}
```

## Audio Configuration

### Audio Settings

```json
{
  "audio": {
    "input": {
      "device": "default",
      "sample_rate": 44100,
      "bit_depth": 16,
      "channels": 2,
      "buffer_size": 1024,
      "loopback": true
    },
    "analysis": {
      "fft_size": 2048,
      "hop_size": 512,
      "window_type": "hann",
      "sensitivity": 0.8,
      "bpm_range": {
        "min": 80,
        "max": 180
      }
    },
    "features": {
      "enable_spectrum": true,
      "enable_beat_detection": true,
      "enable_genre_classification": true,
      "enable_onset_detection": true
    }
  }
}
```

### Audio Device Selection

```json
{
  "audio": {
    "input": {
      "device": "Microphone (Realtek Audio)",
      "device_id": "{0.0.1.00000000}.{12345678-1234-1234-1234-123456789012}"
    }
  }
}
```

## Video Configuration

### Display Settings

```json
{
  "video": {
    "display": {
      "width": 1920,
      "height": 1080,
      "refresh_rate": 60,
      "fullscreen": false,
      "borderless": true,
      "always_on_top": false,
      "vsync": true,
      "triple_buffering": true
    },
    "rendering": {
      "msaa_samples": 4,
      "anisotropic_filtering": 8,
      "texture_quality": "high",
      "shader_quality": "high"
    },
    "ascii": {
      "grid_width": 160,
      "grid_height": 60,
      "character_set": " .:-=+*#%@",
      "font_size": 16,
      "line_spacing": 1.2
    }
  }
}
```

### Resolution Presets

```json
{
  "video": {
    "presets": {
      "720p": {
        "width": 1280,
        "height": 720,
        "ascii_grid": { "width": 120, "height": 45 }
      },
      "1080p": {
        "width": 1920,
        "height": 1080,
        "ascii_grid": { "width": 160, "height": 60 }
      },
      "4K": {
        "width": 3840,
        "height": 2160,
        "ascii_grid": { "width": 320, "height": 135 }
      },
      "8K": {
        "width": 7680,
        "height": 4320,
        "ascii_grid": { "width": 640, "height": 270 }
      }
    }
  }
}
```

## Performance Configuration

### GPU Settings

```json
{
  "performance": {
    "gpu": {
      "device_index": 0,
      "memory_limit_mb": 2048,
      "compute_threads": 8,
      "workgroup_size": 256,
      "shader_cache": true,
      "validation_layers": false
    },
    "cpu": {
      "thread_count": 4,
      "thread_priority": "normal",
      "affinity_mask": "0xFF"
    },
    "memory": {
      "pool_size_mb": 512,
      "gc_threshold": 0.8,
      "max_allocation_size_mb": 64
    }
  }
}
```

### Frame Rate Limiting

```json
{
  "performance": {
    "framerate": {
      "target_fps": 60,
      "adaptive_fps": true,
      "min_fps": 30,
      "max_fps": 144,
      "fps_measurement_window": 60
    }
  }
}
```

## AI Configuration

### ONNX Model Settings

```json
{
  "ai": {
    "models": {
      "genre_classifier": {
        "path": "models/genre_classifier.onnx",
        "input_shape": [1, 128, 44, 1],
        "output_shape": [1, 10],
        "confidence_threshold": 0.7
      },
      "scene_detector": {
        "path": "models/scene_detector.onnx",
        "input_shape": [1, 256],
        "output_shape": [1, 5],
        "confidence_threshold": 0.6
      }
    },
    "runtime": {
      "execution_provider": "DML",  // DML, CUDA, CPU
      "intra_op_num_threads": 4,
      "inter_op_num_threads": 2,
      "graph_optimization_level": "ORT_ENABLE_ALL"
    }
  }
}
```

### AI Conductor Configuration (LLM Integration)

```json
{
  "llm": {
    "enabled": false,
    "endpoint": "http://host.docker.internal:11434",
    "model": "llama3",
    "max_latency_ms": 12,
    "live_interval_ms": 200,
    "safe_epilepsy": true,
    "musical_analysis": {
      "genre_detection": true,
      "tempo_analysis": true,
      "mood_classification": true,
      "harmonic_analysis": false
    },
    "scene_generation": {
      "context_aware": true,
      "beat_synchronization": true,
      "theme_adaptation": true,
      "safety_filtering": true
    }
  }
}
```

### Safety Configuration

```json
{
  "ai": {
    "safety": {
      "epilepsy_protection": {
        "enabled": true,
        "luminance_threshold": 0.8,
        "flash_frequency_limit": 3.0,
        "transition_smoothing": 0.9
      },
      "content_filter": {
        "enabled": true,
        "nsfw_detection": true,
        "brightness_limit": 0.9,
        "contrast_limit": 2.0
      }
    }
  }
}
```

## Output Configuration

### Broadcasting Settings

```json
{
  "output": {
    "spout": {
      "enabled": true,
      "sender_name": "NeonGlyph",
      "frame_rate": 60,
      "format": "RGBA32"
    },
    "ndi": {
      "enabled": false,
      "sender_name": "NeonGlyph",
      "frame_rate": 60,
      "format": "BGRA",
      "bandwidth": "highest"
    },
    "file": {
      "enabled": false,
      "output_directory": "./recordings",
      "format": "mp4",
      "codec": "h264",
      "bitrate": 8000,
      "quality": "high"
    }
  }
}
```

### Network Streaming

```json
{
  "output": {
    "streaming": {
      "enabled": false,
      "url": "rtmp://localhost:1935/live/stream",
      "protocol": "RTMP",
      "bitrate": 6000,
      "keyframe_interval": 2,
      "preset": "fast"
    }
  }
}
```

## Logging Configuration

### Log Settings

```json
{
  "logging": {
    "level": "info",  // trace, debug, info, warning, error, fatal
    "output": {
      "console": true,
      "file": true,
      "file_path": "logs/neonglyph.log"
    },
    "format": {
      "pattern": "[%Y-%m-%d %H:%M:%S] [%l] %v",
      "enable_colors": true,
      "enable_thread_id": true
    },
    "rotation": {
      "enabled": true,
      "max_size_mb": 10,
      "max_files": 5,
      "compress": true
    }
  }
}
```

### Module-Specific Logging

```json
{
  "logging": {
    "modules": {
      "audio": "info",
      "video": "info",
      "ai": "warning",
      "renderer": "info",
      "output": "info"
    }
  }
}
```

## System Integration

### Windows Integration

```json
{
  "system": {
    "windows": {
      "auto_start": false,
      "start_minimized": true,
      "system_tray": true,
      "jump_list": true,
      "notifications": true,
      "high_dpi": true,
      "dpi_awareness": "per_monitor_v2"
    }
  }
}
```

### Hotkey Configuration

```json
{
  "system": {
    "hotkeys": {
      "toggle_fullscreen": "F11",
      "toggle_playback": "Space",
      "next_theme": "Ctrl+T",
      "open_settings": "Ctrl+,",
      "exit": "Alt+F4",
      "emergency_stop": "Ctrl+Shift+Esc"
    }
  }
}
```

## Environment Variables

### Override Configuration

```bash
# Application settings
export NEONGLYPH_LOG_LEVEL=debug
export NEONGLYPH_CONFIG_PATH=/custom/path/config.json

# Audio settings
export NEONGLYPH_AUDIO_DEVICE="Microphone (Realtek Audio)"
export NEONGLYPH_SAMPLE_RATE=48000

# Video settings
export NEONGLYPH_WIDTH=2560
export NEONGLYPH_HEIGHT=1440
export NEONGLYPH_FULLSCREEN=false

# Performance settings
export NEONGLYPH_GPU_INDEX=1
export NEONGLYPH_THREAD_COUNT=8
export NEONGLYPH_MEMORY_LIMIT_MB=4096
```

### Command Line Arguments

```bash
# Basic usage
./NeonGlyph --config custom_config.json

# Override specific settings
./NeonGlyph --width 1920 --height 1080 --fullscreen

# Debug mode
./NeonGlyph --log-level debug --validation-layers

# Performance profiling
./NeonGlyph --profile --benchmark --fps-cap 144

# AI Conductor configuration
./NeonGlyph --llm-endpoint http://localhost:11434 --llm-model llama3

# Audio device selection
./NeonGlyph --audio-device "Microphone (Realtek Audio)"

# ASCII character set
./NeonGlyph --charset "@%#*+=-:. " --brightness 0.8 --contrast 1.2
```

## Configuration Validation

### Schema Validation

```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "object",
  "properties": {
    "application": {
      "type": "object",
      "properties": {
        "log_level": {
          "enum": ["trace", "debug", "info", "warning", "error", "fatal"]
        }
      }
    }
  }
}
```

### Runtime Validation

```cpp
// Validate configuration
ConfigValidator validator;
if (!validator.Validate(config_json)) {
    std::cerr << "Configuration validation failed: " 
              << validator.GetErrorMessage() << std::endl;
    return false;
}
```

## Configuration Management

### Loading Configuration

```cpp
#include "ConfigManager.h"

// Load from file
ConfigManager config;
if (!config.LoadFromFile("config/default.json")) {
    std::cerr << "Failed to load configuration" << std::endl;
    return -1;
}

// Access configuration values
std::string log_level = config.GetString("logging.level");
uint32_t width = config.GetUInt("video.width");
float brightness = config.GetFloat("theme.ascii.brightness");
```

### Hot Reloading

```json
{
  "application": {
    "hot_reload": {
      "enabled": true,
      "watch_files": ["config/themes/*.json"],
      "check_interval": 5000
    }
  }
}
```

## Troubleshooting

### Common Configuration Issues

1. **Invalid JSON Format**
   ```json
   // Incorrect (trailing comma)
   {
     "key": "value",
   }
   
   // Correct
   {
     "key": "value"
   }
   ```

2. **Invalid Values**
   ```json
   // Incorrect (string instead of number)
   {
     "width": "1920"
   }
   
   // Correct
   {
     "width": 1920
   }
   ```

3. **Missing Required Fields**
   ```json
   // Ensure all required fields are present
   {
     "audio": {
       "sample_rate": 44100,  // Required
       "buffer_size": 1024    // Required
     }
   }
   ```

### Configuration Debugging

```bash
# Enable configuration debug logging
export NEONGLYPH_CONFIG_DEBUG=true

# Validate configuration file
./NeonGlyph --validate-config config.json

# Generate default configuration
./NeonGlyph --generate-config > default_config.json
```

## References

- [JSON Schema Documentation](https://json-schema.org/)
- [NeonGlyph Build Guide](build-guide.md)
- [NeonGlyph Architecture Overview](architecture-overview.md)

## Change History

### Version 2.1.0 (2025-11-13)
- Added AI Conductor (LLM) configuration section
- Enhanced main configuration with hot-reload settings
- Updated default audio sample rate to 48kHz
- Added window configuration section with 144 FPS target
- Enhanced command line arguments with AI Conductor options
- Added ASCII character set and audio device selection parameters

### Version 2.0.0 (2024-11-13)
- Complete configuration documentation rewrite
- Added comprehensive theme configuration
- Enhanced performance and AI configuration sections
- Added environment variables and command line documentation

### Version 1.0.0 (2024-01-01)
- Initial configuration guide creation
- Basic configuration options documentation
*** End of File