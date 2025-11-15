#!/usr/bin/env python3
"""
Seamless Scaling Demonstration for NeonGlyph
Perfect borderless scaling that adapts to any monitor size
"""

import tkinter as tk
from tkinter import font
import math
import time
import threading
import random
import win32api
import win32con
import win32gui
import win32ui
from ctypes import windll
import numpy as np

class SeamlessScalingDemo:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("")
        self.root.configure(bg='black')
        
        # Remove all window decorations and borders
        self.root.overrideredirect(True)
        self.root.attributes('-topmost', True)
        self.root.attributes('-transparentcolor', 'black')
        
        # Get primary monitor dimensions
        self.monitor_info = self.get_primary_monitor()
        self.screen_width = self.monitor_info['width']
        self.screen_height = self.monitor_info['height']
        
        # Set window to exact screen dimensions
        self.root.geometry(f"{self.screen_width}x{self.screen_height}+0+0")
        
        # Create canvas for seamless visuals
        self.canvas = tk.Canvas(
            self.root,
            width=self.screen_width,
            height=self.screen_height,
            bg='black',
            highlightthickness=0,
            bd=0
        )
        self.canvas.pack(fill='both', expand=True)
        
        # ASCII art parameters
        self.ascii_chars = "@#S%?*+;:,. "
        self.char_width = 8
        self.char_height = 16
        
        # Calculate optimal character density for current monitor
        self.chars_x = self.screen_width // self.char_width
        self.chars_y = self.screen_height // self.char_height
        
        # Visual parameters
        self.time_offset = 0
        self.audio_reactivity = 0.5
        self.running = True
        
        # Font setup for crisp ASCII
        self.setup_font()
        
        # Start seamless animation
        self.start_animation()
        
        # Bind escape to exit
        self.root.bind('<Escape>', lambda e: self.exit_fullscreen())
        
        # Make window truly borderless using Windows API
        self.make_borderless()
        
    def get_primary_monitor(self):
        """Get primary monitor dimensions"""
        # Get device caps
        user32 = windll.user32
        user32.SetProcessDPIAware()
        
        width = user32.GetSystemMetrics(0)
        height = user32.GetSystemMetrics(1)
        
        return {
            'width': width,
            'height': height,
            'aspect_ratio': width / height
        }
    
    def setup_font(self):
        """Setup perfect monospace font for scaling"""
        try:
            # Try to use a crisp monospace font
            self.font = font.Font(
                family="Consolas",
                size=10,
                weight="normal"
            )
        except:
            # Fallback to default monospace
            self.font = font.Font(
                family="Courier",
                size=10,
                weight="normal"
            )
    
    def make_borderless(self):
        """Make window truly borderless using Windows API"""
        hwnd = self.root.winfo_id()
        
        # Remove window decorations completely
        style = win32gui.GetWindowLong(hwnd, win32con.GWL_STYLE)
        style &= ~(win32con.WS_CAPTION | win32con.WS_THICKFRAME | 
                  win32con.WS_MINIMIZE | win32con.WS_MAXIMIZE | 
                  win32con.WS_SYSMENU | win32con.WS_BORDER)
        win32gui.SetWindowLong(hwnd, win32con.GWL_STYLE, style)
        
        # Set extended styles for seamless appearance
        ex_style = win32gui.GetWindowLong(hwnd, win32con.GWL_EXSTYLE)
        ex_style |= (win32con.WS_EX_TOOLWINDOW | win32con.WS_EX_TOPMOST)
        ex_style &= ~win32con.WS_EX_APPWINDOW  # Hide from taskbar
        win32gui.SetWindowLong(hwnd, win32con.GWL_EXSTYLE, ex_style)
        
        # Set window as layered for transparency
        win32gui.SetWindowLong(hwnd, win32con.GWL_EXSTYLE, 
                              ex_style | win32con.WS_EX_LAYERED)
        
        # Make window click-through (optional - uncomment if needed)
        # win32gui.SetWindowLong(hwnd, win32con.GWL_EXSTYLE,
        #                       ex_style | win32con.WS_EX_TRANSPARENT)
    
    def generate_ascii_pattern(self, x, y, time_val):
        """Generate seamless ASCII pattern with perfect scaling"""
        # Base cyberpunk @ symbol pattern
        base_char = "@"
        
        # Create flowing wave patterns that scale perfectly
        wave_x = math.sin((x / self.chars_x) * math.pi * 4 + time_val * 0.5)
        wave_y = math.cos((y / self.chars_y) * math.pi * 3 + time_val * 0.3)
        wave_diag = math.sin((x + y) / (self.chars_x + self.chars_y) * math.pi * 5 + time_val * 0.7)
        
        # Combine waves for complex pattern
        combined_wave = (wave_x + wave_y + wave_diag) / 3
        
        # Add audio reactivity
        intensity = combined_wave * self.audio_reactivity
        
        # Map to ASCII brightness
        char_index = int((intensity + 1) * len(self.ascii_chars) / 2)
        char_index = max(0, min(len(self.ascii_chars) - 1, char_index))
        
        return self.ascii_chars[char_index]
    
    def get_cyberpunk_color(self, intensity, x, y):
        """Generate cyberpunk color palette that scales perfectly"""
        # Base neon colors
        neon_cyan = (0, 255, 255)
        neon_purple = (255, 0, 255)
        neon_green = (0, 255, 0)
        
        # Create gradient based on position and intensity
        gradient_pos = (x / self.chars_x + y / self.chars_y) / 2
        
        # Interpolate between colors
        if gradient_pos < 0.33:
            factor = gradient_pos / 0.33
            r = int(neon_cyan[0] * (1 - factor) + neon_purple[0] * factor)
            g = int(neon_cyan[1] * (1 - factor) + neon_purple[1] * factor)
            b = int(neon_cyan[2] * (1 - factor) + neon_purple[2] * factor)
        elif gradient_pos < 0.66:
            factor = (gradient_pos - 0.33) / 0.33
            r = int(neon_purple[0] * (1 - factor) + neon_green[0] * factor)
            g = int(neon_purple[1] * (1 - factor) + neon_green[1] * factor)
            b = int(neon_purple[2] * (1 - factor) + neon_green[2] * factor)
        else:
            factor = (gradient_pos - 0.66) / 0.34
            r = int(neon_green[0] * (1 - factor) + neon_cyan[0] * factor)
            g = int(neon_green[1] * (1 - factor) + neon_cyan[1] * factor)
            b = int(neon_green[2] * (1 - factor) + neon_cyan[2] * factor)
        
        # Apply intensity
        brightness = abs(intensity) * 0.8 + 0.2
        r = int(r * brightness)
        g = int(g * brightness)
        b = int(b * brightness)
        
        return f"#{r:02x}{g:02x}{b:02x}"
    
    def update_frame(self):
        """Update frame with seamless scaling"""
        # Clear canvas completely
        self.canvas.delete("all")
        
        # Update time
        self.time_offset += 0.1
        
        # Simulate audio reactivity
        self.audio_reactivity = 0.3 + 0.2 * math.sin(self.time_offset * 2)
        
        # Generate seamless ASCII field
        for y in range(0, self.chars_y, 2):  # Skip lines for performance
            for x in range(0, self.chars_x, 1):
                # Generate character with perfect scaling
                char = self.generate_ascii_pattern(x, y, self.time_offset)
                intensity = math.sin((x / self.chars_x) * math.pi * 4 + self.time_offset * 0.5)
                
                # Get cyberpunk color
                color = self.get_cyberpunk_color(intensity, x, y)
                
                # Calculate exact pixel position
                pixel_x = x * self.char_width
                pixel_y = y * self.char_height
                
                # Draw character with perfect positioning
                self.canvas.create_text(
                    pixel_x,
                    pixel_y,
                    text=char,
                    fill=color,
                    font=self.font,
                    anchor='nw'
                )
        
        # Update canvas
        self.canvas.update()
    
    def start_animation(self):
        """Start seamless animation loop"""
        def animate():
            while self.running:
                try:
                    self.update_frame()
                    time.sleep(0.033)  # ~30 FPS for smooth scaling
                except Exception as e:
                    print(f"Animation error: {e}")
                    break
        
        # Start animation in separate thread
        self.animation_thread = threading.Thread(target=animate, daemon=True)
        self.animation_thread.start()
    
    def exit_fullscreen(self):
        """Exit seamless mode"""
        self.running = False
        self.root.quit()
        self.root.destroy()

def main():
    """Main function to run seamless scaling demo"""
    print("Starting NeonGlyph Seamless Scaling Demo...")
    print("Press ESC to exit")
    
    try:
        demo = SeamlessScalingDemo()
        demo.root.mainloop()
    except KeyboardInterrupt:
        print("\nDemo interrupted by user")
    except Exception as e:
        print(f"Demo error: {e}")
    finally:
        print("Demo completed")

if __name__ == "__main__":
    main()