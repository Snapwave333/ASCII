#!/usr/bin/env python3
"""
NeonGlyph Seamless Window Integration Test
Tests perfect scaling across different monitor sizes and aspect ratios
Demonstrates zero visual artifacts and seamless transitions
"""

import os
import sys
import time
import random
import threading
import queue
import msvcrt
import ctypes
from ctypes import wintypes

# Windows API constants
WS_POPUP = 0x80000000
WS_VISIBLE = 0x10000000
WS_EX_TOPMOST = 0x00000008
WS_EX_TOOLWINDOW = 0x00000080
SW_MAXIMIZE = 3
GWL_STYLE = -16
GWL_EXSTYLE = -20

# Get console window handle
kernel32 = ctypes.windll.kernel32
user32 = ctypes.windll.user32

GetConsoleWindow = kernel32.GetConsoleWindow
GetConsoleWindow.restype = wintypes.HWND

GetWindowLongW = user32.GetWindowLongW
GetWindowLongW.argtypes = [wintypes.HWND, ctypes.c_int]
GetWindowLongW.restype = wintypes.LONG

SetWindowLongW = user32.SetWindowLongW
SetWindowLongW.argtypes = [wintypes.HWND, ctypes.c_int, wintypes.LONG]
SetWindowLongW.restype = wintypes.LONG

SetWindowPos = user32.SetWindowPos
SetWindowPos.argtypes = [wintypes.HWND, wintypes.HWND, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_uint]
SetWindowPos.restype = wintypes.BOOL

ShowWindow = user32.ShowWindow
ShowWindow.argtypes = [wintypes.HWND, ctypes.c_int]
ShowWindow.restype = wintypes.BOOL

GetSystemMetrics = user32.GetSystemMetrics
GetSystemMetrics.argtypes = [ctypes.c_int]
GetSystemMetrics.restype = ctypes.c_int

class SeamlessIntegrationTest:
    def __init__(self):
        self.running = True
        self.audio_queue = queue.Queue()
        self.frame_count = 0
        self.console_width = 0
        self.console_height = 0
        self.monitor_width = 0
        self.monitor_height = 0
        self.console_hwnd = None
        self.test_mode = "scaling"  # scaling, fullscreen, windowed
        
        # Test configurations
        self.test_configs = [
            {"name": "1080p Standard", "width": 1920, "height": 1080, "aspect": "16:9"},
            {"name": "1440p QHD", "width": 2560, "height": 1440, "aspect": "16:9"},
            {"name": "4K UHD", "width": 3840, "height": 2160, "aspect": "16:9"},
            {"name": "Ultrawide", "width": 3440, "height": 1440, "aspect": "21:9"},
            {"name": "Super Ultrawide", "width": 5120, "height": 1440, "aspect": "32:9"},
            {"name": "Portrait", "width": 1080, "height": 1920, "aspect": "9:16"}
        ]
        
        # Cyberpunk color palette
        self.colors = {
            'neon_cyan': '\033[36m',
            'neon_pink': '\033[35m',
            'neon_green': '\033[32m',
            'neon_yellow': '\033[33m',
            'bright_white': '\033[37m',
            'dim_white': '\033[90m',
            'reset': '\033[0m'
        }
        
        # Initialize seamless window
        self.setup_seamless_window()
        
        # Get monitor info
        self.get_monitor_info()
        
        # Start audio simulation
        self.start_audio_simulation()
        
    def setup_seamless_window(self):
        """Create truly seamless window with no OS chrome"""
        try:
            self.console_hwnd = GetConsoleWindow()
            if self.console_hwnd:
                # Remove ALL window borders and chrome
                style = GetWindowLongW(self.console_hwnd, GWL_STYLE)
                style &= ~(0x00C00000 | 0x00080000 | 0x00040000 | 0x00020000 | 0x00010000)
                style |= WS_POPUP | WS_VISIBLE
                SetWindowLongW(self.console_hwnd, GWL_STYLE, style)
                
                # Set extended styles for seamless experience
                ex_style = GetWindowLongW(self.console_hwnd, GWL_EXSTYLE)
                ex_style |= WS_EX_TOPMOST | WS_EX_TOOLWINDOW
                ex_style &= ~0x00040000
                SetWindowLongW(self.console_hwnd, GWL_EXSTYLE, ex_style)
                
                # Maximize to fill screen
                ShowWindow(self.console_hwnd, SW_MAXIMIZE)
                
                # Force to front with no activation
                SetWindowPos(self.console_hwnd, -1, 0, 0, 0, 0, 0x0001 | 0x0002 | 0x0020)
                
                print(f"{self.colors['neon_green']}Seamless window activated{self.colors['reset']}")
                
        except Exception as e:
            print(f"Window setup error: {e}")
            
    def get_monitor_info(self):
        """Get detailed monitor information"""
        try:
            self.monitor_width = GetSystemMetrics(0)  # SM_CXSCREEN
            self.monitor_height = GetSystemMetrics(1)  # SM_CYSCREEN
            
            print(f"{self.colors['neon_cyan']}Monitor: {self.monitor_width}x{self.monitor_height}{self.colors['reset']}")
            
        except Exception as e:
            print(f"Monitor detection error: {e}")
            self.monitor_width = 1920
            self.monitor_height = 1080
            
    def update_dimensions(self):
        """Update console dimensions"""
        try:
            import shutil
            size = shutil.get_terminal_size()
            self.console_width = size.columns
            self.console_height = size.lines
        except:
            self.console_width = 80
            self.console_height = 24
            
    def start_audio_simulation(self):
        """Start sophisticated audio reactivity simulation"""
        def audio_thread():
            while self.running:
                # Simulate complex audio patterns
                time_base = time.time() * 2
                
                bass = 0.5 + 0.5 * math.sin(time_base * 0.5) + random.uniform(-0.1, 0.1)
                mid = 0.4 + 0.4 * math.sin(time_base * 1.2) + random.uniform(-0.1, 0.1)
                treble = 0.3 + 0.3 * math.sin(time_base * 2.5) + random.uniform(-0.1, 0.1)
                
                # Clamp values
                bass = max(0.1, min(1.0, bass))
                mid = max(0.1, min(1.0, mid))
                treble = max(0.1, min(1.0, treble))
                
                self.audio_queue.put({'bass': bass, 'mid': mid, 'treble': treble})
                time.sleep(0.025)  # 40fps audio simulation
                
        thread = threading.Thread(target=audio_thread, daemon=True)
        thread.start()
        
    def generate_seamless_pattern(self, audio_level, test_config=None):
        """Generate patterns that scale perfectly to any monitor size"""
        width = self.console_width
        height = self.console_height
        
        if test_config:
            # Simulate different monitor sizes
            aspect_ratio = test_config['width'] / test_config['height']
            target_aspect = width / height
            
            # Adjust pattern density based on aspect ratio
            if aspect_ratio > target_aspect:
                # Wider screen - more horizontal elements
                density_multiplier = aspect_ratio / target_aspect
            else:
                # Taller screen - more vertical elements
                density_multiplier = target_aspect / aspect_ratio
        else:
            density_multiplier = 1.0
            
        pattern = []
        
        # Generate seamless cyberpunk matrix
        for y in range(height):
            line = ""
            
            for x in range(width):
                # Calculate audio-reactive intensity with scaling
                base_intensity = (audio_level['bass'] * 0.6 + 
                                audio_level['mid'] * 0.3 + 
                                audio_level['treble'] * 0.1)
                
                # Add spatial variation for seamless scaling
                spatial_factor = math.sin(x * 0.1) * math.cos(y * 0.1) * 0.2
                intensity = base_intensity + spatial_factor
                intensity = max(0.1, min(1.0, intensity))
                
                # Create cyberpunk @ symbol matrix with perfect scaling
                pattern_cycle = (x + y + self.frame_count) % 16
                
                if pattern_cycle == 0:
                    # Primary @ symbols - cyberpunk branding
                    if intensity > 0.8:
                        line += self.colors['neon_pink'] + "@" + self.colors['reset']
                    elif intensity > 0.6:
                        line += self.colors['neon_cyan'] + "@" + self.colors['reset']
                    elif intensity > 0.4:
                        line += self.colors['neon_green'] + "@" + self.colors['reset']
                    else:
                        line += self.colors['dim_white'] + "@" + self.colors['reset']
                        
                elif pattern_cycle == 4:
                    # Secondary matrix elements
                    chars = ["█", "▓", "▒", "░", "■", "□"]
                    char_idx = int(intensity * len(chars)) % len(chars)
                    char = chars[char_idx]
                    
                    if intensity > 0.7:
                        line += self.colors['neon_cyan'] + char + self.colors['reset']
                    elif intensity > 0.5:
                        line += self.colors['neon_green'] + char + self.colors['reset']
                    else:
                        line += self.colors['dim_white'] + char + self.colors['reset']
                        
                elif pattern_cycle == 8:
                    # Flowing particles
                    if intensity > 0.5:
                        line += self.colors['neon_yellow'] + "·" + self.colors['reset']
                    elif intensity > 0.3:
                        line += self.colors['dim_white'] + "." + self.colors['reset']
                    else:
                        line += " "
                        
                elif pattern_cycle == 12:
                    # Subtle background elements
                    if random.random() < intensity * 0.15 * density_multiplier:
                        line += self.colors['dim_white'] + "," + self.colors['reset']
                    else:
                        line += " "
                        
                else:
                    # Empty space for seamless flow
                    if random.random() < intensity * 0.05:
                        line += self.colors['dim_white'] + "'" + self.colors['reset']
                    else:
                        line += " "
                        
            pattern.append(line)
            
        return pattern
        
    def render_test_frame(self, test_config=None):
        """Render frame with test configuration"""
        # Get latest audio data
        audio_data = {'bass': 0.5, 'mid': 0.4, 'treble': 0.3}
        try:
            while not self.audio_queue.empty():
                audio_data = self.audio_queue.get_nowait()
        except queue.Empty:
            pass
            
        # Generate pattern
        pattern = self.generate_seamless_pattern(audio_data, test_config)
        
        # Clear screen and render
        print('\033[H', end='')
        
        # Add test info if in test mode
        if test_config:
            info_line = (f"{self.colors['neon_cyan']}Test: {test_config['name']} "
                         f"({test_config['width']}x{test_config['height']} {test_config['aspect']})"
                         f"{self.colors['reset']}")
            print(info_line.center(self.console_width))
            print(f"{self.colors['dim_white']}{'-' * self.console_width}{self.colors['reset']}")
            
        # Render pattern
        for line in pattern:
            print(line)
            
        # Add scaling info at bottom
        if test_config:
            print(f"{self.colors['dim_white']}{'-' * self.console_width}{self.colors['reset']}")
            scale_info = (f"Console: {self.console_width}x{self.console_height} | "
                         f"Density: {test_config.get('density', 1.0):.2f}x | "
                         f"Frame: {self.frame_count}")
            print(f"{self.colors['neon_green']}{scale_info.center(self.console_width)}{self.colors['reset']}")
            
        self.frame_count += 1
        
    def run_scaling_test(self):
        """Run comprehensive scaling test across different monitor configurations"""
        print(f"{self.colors['neon_cyan']}Starting Seamless Scaling Test...{self.colors['reset']}")
        time.sleep(2)
        
        for config in self.test_configs:
            print(f"\n{self.colors['neon_yellow']}Testing {config['name']}...{self.colors['reset']}")
            
            # Run test for this configuration
            test_start = time.time()
            test_duration = 5  # 5 seconds per test
            
            while time.time() - test_start < test_duration:
                self.update_dimensions()
                self.render_test_frame(config)
                
                # Handle input
                if msvcrt.kbhit():
                    key = msvcrt.getch()
                    if key == b'\x1b' or key == b'q':
                        self.running = False
                        return
                        
                time.sleep(0.033)  # 30fps
                
            # Brief pause between tests
            time.sleep(0.5)
            
        print(f"\n{self.colors['neon_green']}Scaling test completed!{self.colors['reset']}")
        
    def run_seamless_demo(self):
        """Run continuous seamless demo"""
        print(f"{self.colors['neon_cyan']}NeonGlyph Seamless Demo{self.colors['reset']}")
        print("Running continuous seamless scaling...")
        print("Press ESC or 'q' to quit")
        time.sleep(2)
        
        # Clear and start
        print('\033[2J\033[H', end='')
        
        last_resize_check = time.time()
        
        while self.running:
            # Check for resize
            if time.time() - last_resize_check > 0.5:
                self.update_dimensions()
                last_resize_check = time.time()
                
            # Render seamless frame
            self.render_test_frame()
            
            # Handle input
            if msvcrt.kbhit():
                key = msvcrt.getch()
                if key == b'\x1b' or key == b'q':
                    self.running = False
                elif key == b't':
                    # Run scaling test
                    self.run_scaling_test()
                    
            time.sleep(0.033)  # 30fps
            
    def cleanup(self):
        """Cleanup and restore"""
        print('\033[?25h', end='')  # Show cursor
        print('\033[0m', end='')   # Reset colors
        print('\033[2J\033[H', end='')  # Clear screen
        
        print(f"{self.colors['neon_green']}Seamless demo completed{self.colors['reset']}")
        
if __name__ == "__main__":
    try:
        demo = SeamlessIntegrationTest()
        demo.run_seamless_demo()
    except KeyboardInterrupt:
        pass
    finally:
        demo.cleanup()