# NEON-GLYPH HEADLESS MODE USAGE GUIDE
## Practical Deployment Scenarios

## Quick Start

### 1. Force Headless Mode
```bash
# Run in headless mode (no window)
./NeonGlyph --headless

# With enhanced logging
./NeonGlyph --headless --enable-headless-logging
```

### 2. Automatic Fallback
```bash
# Normal mode with automatic fallback enabled (default)
./NeonGlyph

# Disable automatic fallback
./NeonGlyph --no-headless-fallback
```

### 3. Configuration File
```bash
# Use custom headless configuration
./NeonGlyph --headless --config config/headless_config.json
```

## Deployment Scenarios

### Scenario 1: Server Deployment
**Environment**: Linux server without display
**Challenge**: No X11/Wayland display server available
**Solution**: Force headless mode

```bash
# Check if display is available
if [ -z "$DISPLAY" ]; then
    echo "No display detected, running in headless mode"
    ./NeonGlyph --headless --config config/server_config.json
else
    echo "Display available, running normally"
    ./NeonGlyph
fi
```

**Systemd Service**:
```ini
[Unit]
Description=NeonGlyph Audio Visualizer
After=network.target

[Service]
Type=simple
User=neonglyph
WorkingDirectory=/opt/neonglyph
ExecStart=/opt/neonglyph/NeonGlyph --headless --config config/headless_config.json
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

### Scenario 2: Docker Container
**Environment**: Containerized deployment
**Challenge**: No display server in container
**Solution**: Headless mode with volume mounts

**Dockerfile**:
```dockerfile
FROM ubuntu:20.04

RUN apt-get update && apt-get install -y \
    libglfw3 \
    libvulkan1 \
    libasound2 \
    libpulse0 \
    && rm -rf /var/lib/apt/lists/*

COPY build/NeonGlyph /app/
COPY config/headless_config.json /app/config/

WORKDIR /app

CMD ["./NeonGlyph", "--headless", "--config", "config/headless_config.json"]
```

**Docker Compose**:
```yaml
version: '3.8'
services:
  neonglyph:
    build: .
    container_name: neonglyph-headless
    restart: unless-stopped
    volumes:
      - ./audio_input:/app/audio_input
      - ./logs:/app/logs
    environment:
      - NEONGLYPH_HEADLESS=1
      - NEONGLYPH_HEADLESS_LOGGING=1
    devices:
      - /dev/snd:/dev/snd
```

### Scenario 3: SSH/Remote Access
**Environment**: Remote server via SSH
**Challenge**: No display forwarding available
**Solution**: Automatic fallback detection

```bash
# Check if we're in an SSH session
if [ -n "$SSH_CLIENT" ] || [ -n "$SSH_TTY" ]; then
    echo "SSH session detected, using headless mode"
    ./NeonGlyph --headless
else
    echo "Local session, attempting normal mode"
    ./NeonGlyph
fi
```

### Scenario 4: CI/CD Pipeline
**Environment**: GitHub Actions/Jenkins
**Challenge**: Headless build environment
**Solution**: Force headless mode with test validation

**GitHub Actions**:
```yaml
name: Test Headless Mode

on: [push, pull_request]

jobs:
  test-headless:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v2
    
    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y libglfw3-dev libvulkan-dev
    
    - name: Build application
      run: |
        mkdir build
        cd build
        cmake ..
        make -j4
    
    - name: Test headless mode
      run: |
        timeout 30s ./build/NeonGlyph --headless --enable-headless-logging || true
        
    - name: Test fallback mechanism
      run: |
        # Force display failure and test fallback
        unset DISPLAY
        timeout 30s ./build/NeonGlyph --enable-headless-logging || true
```

### Scenario 5: Multi-GPU Systems
**Environment**: Systems with multiple GPUs
**Challenge**: Vulkan initialization crashes
**Solution**: Enhanced error detection with automatic fallback

```bash
# Run with automatic multi-GPU safety
./NeonGlyph

# If crashes occur, fallback automatically activates
# Check logs for: "[HEADLESS MODE ACTIVATED]"
```

### Scenario 6: Remote Desktop (Windows)
**Environment**: Windows Remote Desktop
**Challenge**: WDDM display driver issues
**Solution**: Automatic fallback detection

```batch
@echo off
REM Check if running in Remote Desktop session
query session %username% | find "rdp-tcp"
if %errorlevel% == 0 (
    echo Remote Desktop session detected, using headless mode
    NeonGlyph.exe --headless
) else (
    echo Local session, running normally
    NeonGlyph.exe
)
```

## Configuration Examples

### Basic Headless Configuration
```json
{
    "headless": {
        "enabled": true,
        "fallback": false,
        "logActivation": true
    },
    "audio": {
        "sampleRate": 48000,
        "channels": 2,
        "bufferSize": 1024
    }
}
```

### Server Deployment Configuration
```json
{
    "headless": {
        "enabled": true,
        "fallback": true,
        "logActivation": true
    },
    "audio": {
        "sampleRate": 48000,
        "channels": 2,
        "bufferSize": 2048,
        "deviceId": "default"
    },
    "output": {
        "spoutEnabled": false,
        "ndiEnabled": true,
        "ndiName": "NeonGlyph-Server"
    },
    "safety": {
        "photosensitiveMode": true,
        "crashRecovery": true
    }
}
```

### Development/Testing Configuration
```json
{
    "headless": {
        "enabled": false,
        "fallback": true,
        "logActivation": true
    },
    "audio": {
        "sampleRate": 44100,
        "channels": 2,
        "bufferSize": 512
    },
    "render": {
        "overlayEnabled": true,
        "targetFPS": 60
    }
}
```

## Monitoring and Debugging

### Log Analysis
```bash
# Monitor headless mode activation
tail -f logs/neonglyph.log | grep "HEADLESS"

# Check for fallback events
grep "HEADLESS_MODE_ACTIVATED" logs/neonglyph.log

# Monitor performance metrics
tail -f logs/neonglyph.log | grep "HEADLESS_METRIC"
```

### Performance Monitoring
```bash
# Monitor CPU usage in headless mode
top -p $(pgrep NeonGlyph)

# Check memory usage
ps aux | grep NeonGlyph

# Monitor audio processing
pactl list source-outputs | grep NeonGlyph
```

### Health Checks
```bash
#!/bin/bash
# Health check script for headless mode

PROCESS_NAME="NeonGlyph"
LOG_FILE="logs/health.log"

# Check if process is running
if pgrep -x "$PROCESS_NAME" > /dev/null; then
    echo "$(date): $PROCESS_NAME is running" >> $LOG_FILE
    
    # Check recent log activity
    if tail -n 100 logs/neonglyph.log | grep -q "HEADLESS"; then
        echo "$(date): Headless mode active" >> $LOG_FILE
    else
        echo "$(date): WARNING - No headless activity detected" >> $LOG_FILE
    fi
else
    echo "$(date): ERROR - $PROCESS_NAME is not running" >> $LOG_FILE
    # Attempt restart
    systemctl restart neonglyph
fi
```

## Troubleshooting

### Common Issues

#### 1. Headless Mode Not Activating
**Symptoms**: Application crashes without fallback
**Solution**: Check configuration and logs
```bash
# Enable verbose logging
./NeonGlyph --enable-headless-logging --log-level debug

# Check for fallback configuration
grep "fallback" config/neonglyph.json
```

#### 2. Audio Not Working in Headless Mode
**Symptoms**: No audio input detected
**Solution**: Check audio permissions and devices
```bash
# List audio devices
pactl list sources

# Test audio input
arecord -l

# Check permissions
sudo usermod -a -G audio $USER
```

#### 3. Performance Issues in Headless Mode
**Symptoms**: High CPU usage or frame drops
**Solution**: Optimize configuration
```json
{
    "audio": {
        "bufferSize": 2048,
        "sampleRate": 44100
    },
    "render": {
        "targetFPS": 30
    }
}
```

#### 4. Vulkan Initialization Failures
**Symptoms**: Multi-GPU crashes
**Solution**: Use headless mode or environment fixes
```bash
# Force headless mode
./NeonGlyph --headless

# Or use automatic fallback (default)
./NeonGlyph
```

### Environment-Specific Fixes

#### Docker Audio Permissions
```dockerfile
# Add audio group permissions
RUN groupadd -g 29 audio && usermod -a -G audio neonglyph

# Grant audio device access
VOLUME ["/dev/snd:/dev/snd"]
```

#### Systemd Service Issues
```ini
[Service]
# Add audio group
SupplementaryGroups=audio

# Grant device access
DeviceAllow=/dev/snd rw
```

## Best Practices

### 1. Always Test Fallback Mechanism
```bash
# Test by forcing display failure
unset DISPLAY
./NeonGlyph --enable-headless-logging
```

### 2. Monitor Resource Usage
```bash
# Set up monitoring scripts
./monitor_performance.sh &
./check_health.sh &
```

### 3. Use Appropriate Configuration
- **Server**: High buffer sizes, reduced FPS
- **Development**: Enhanced logging, normal FPS
- **Production**: Minimal logging, optimized settings

### 4. Implement Proper Logging
```bash
# Rotate logs regularly
logrotate -f /etc/logrotate.d/neonglyph

# Monitor for errors
grep -i error logs/neonglyph.log
```

### 5. Plan for Recovery
```bash
# Automatic restart on failure
while true; do
    ./NeonGlyph --headless --config config/headless_config.json
    echo "Restarting in 10 seconds..."
    sleep 10
done
```

## Conclusion

The enhanced headless fallback system ensures NeonGlyph operates reliably across all deployment scenarios. By following these guidelines and configurations, you can deploy the application in server environments, containers, remote sessions, and CI/CD pipelines with confidence in its stability and performance.
*** End of File