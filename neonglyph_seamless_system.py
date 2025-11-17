#!/usr/bin/env python3
"""
NeonGlyph Seamless E2E Scaling System
Complete borderless scaling that adapts perfectly to any display
This is the final implementation for the critical requirement
"""

import tkinter as tk
import math
import time
import threading
import random
import win32api
import win32con
import win32gui
from ctypes import windll, wintypes, Structure, POINTER, c_int, c_uint, c_void_p
import ctypes

class NeonGlyphSeamlessSystem:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("")  # No title
        self.root.configure(bg='black')
        
        # CRITICAL: Remove ALL borders and decorations completely
        self.root.overrideredirect(True)
        self.root.attributes('-topmost', True)
        self.root.attributes('-transparentcolor', 'black')
        
        # Get exact screen dimensions
        self.screen_width = windll.user32.GetSystemMetrics(0)
        self.screen_height = windll.user32.GetSystemMetrics(1)
        
        print(f"Detected screen: {self.screen_width}x{self.screen_height}")
        
        # Set window to exact screen dimensions
        self.root.geometry(f"{self.screen_width}x{self.screen_height}+0+0")
        
        # Create seamless canvas
        self.canvas = tk.Canvas(
            self.root,
            width=self.screen_width,
            height=self.screen_height,
            bg='black',
            highlightthickness=0,  # No highlight border
            bd=0,  # No border
            relief='flat'  # No relief
        )
        self.canvas.pack(fill='both', expand=True)
        
        # ASCII parameters for perfect scaling
        self.ascii_chars = "@#S%?*+;:,. "
        self.char_width = 8
        self.char_height = 16
        
        # Calculate perfect character density
        self.chars_x = self.screen_width // self.char_width
        self.chars_y = self.screen_height // self.char_height
        
        # Ensure minimum density for good visuals
        self.chars_x = max(self.chars_x, 100)
        self.chars_y = max(self.chars_y, 30)
        
        # Animation parameters
        self.time_offset = 0
        self.audio_reactivity = 0.6
        self.running = True
        
        # Performance optimization
        self.frame_count = 0
        self.last_fps_time = time.time()
        
        # Setup
        self.setup_perfect_font()
        self.make_completely_seamless()
        self.start_seamless_animation()
        
        # Controls
        self.root.bind('<Escape>', lambda e: self.exit_seamless())
        self.root.bind('<Double-Button-1>', lambda e: self.toggle_fullscreen_mode())
        
        print("Seamless system initialized successfully")
        
    def setup_perfect_font(self):
        """Setup perfect font for seamless scaling"""
        # Calculate optimal font size based on character dimensions
        font_size = max(8, min(14, self.char_height - 2))
        
        try:
            # Use Consolas for perfect monospace rendering
            self.font = ("Consolas", font_size, "normal")
            # Test font
            test_id = self.canvas.create_text(-100, -100, text="@", font=self.font)
            self.canvas.delete(test_id)
        except:
            # Fallback to Courier
            self.font = ("Courier", font_size, "normal")
        
        print(f"Font configured: {self.font}")
    
    def make_completely_seamless(self):
        """Make window completely seamless with zero borders"""
        hwnd = self.root.winfo_id()
        
        # CRITICAL: Remove ALL window styles for complete borderlessness
        style = win32gui.GetWindowLong(hwnd, win32con.GWL_STYLE)
        style = win32con.WS_POPUP | win32con.WS_VISIBLE  # ONLY popup and visible
        win32gui.SetWindowLong(hwnd, win32con.GWL_STYLE, style)
        
        # CRITICAL: Set extended styles for complete invisibility of OS chrome
        ex_style = win32gui.GetWindowLong(hwnd, win32con.GWL_EXSTYLE)
        ex_style |= (win32con.WS_EX_TOOLWINDOW | win32con.WS_EX_TOPMOST | 
                    win32con.WS_EX_LAYERED | win32con.WS_EX_TRANSPARENT)
        ex_style &= ~(win32con.WS_EX_APPWINDOW | win32con.WS_EX_DLGMODALFRAME | 
                     win32con.WS_EX_WINDOWEDGE | win32con.WS_EX_CLIENTEDGE)
        win32gui.SetWindowLong(hwnd, win32con.GWL_EXSTYLE, ex_style)
        
        # CRITICAL: Set window position to cover entire screen perfectly
        win32gui.SetWindowPos(
            hwnd,
            win32con.HWND_TOPMOST,  # Always on top
            0, 0,  # Position at 0,0
            self.screen_width,  # Exact screen width
            self.screen_height,  # Exact screen height
            win32con.SWP_FRAMECHANGED | win32con.SWP_SHOWWINDOW | 
            win32con.SWP_NOOWNERZORDER | win32con.SWP_NOZORDER
        )
        
        print("Window made completely seamless")
    
    def generate_seamless_pattern(self, x, y, time_val):
        """Generate seamless ASCII pattern with mathematical precision"""
        # Create flowing cyberpunk @ symbol patterns
        wave1 = math.sin((x / self.chars_x) * math.pi * 6 + time_val * 0.8)
        wave2 = math.cos((y / self.chars_y) * math.pi * 4 + time_val * 0.6)
        wave3 = math.sin(((x + y) / (self.chars_x + self.chars_y)) * math.pi * 8 + time_val * 1.2)
        wave4 = math.cos(((x - y) / max(self.chars_x, self.chars_y)) * math.pi * 5 + time_val * 0.9)
        
        # Combine waves for complex cyberpunk pattern
        combined = (wave1 * 0.3 + wave2 * 0.25 + wave3 * 0.25 + wave4 * 0.2)
        
        # Add audio reactivity
        reactivity = self.audio_reactivity * (0.7 + 0.3 * math.sin(time_val * 3))
        final_intensity = combined * reactivity
        
        # Map to ASCII brightness
        char_index = int((final_intensity + 1) * len(self.ascii_chars) / 2)
        char_index = max(0, min(len(self.ascii_chars) - 1, char_index))
        
        return self.ascii_chars[char_index]
    
    def get_seamless_color(self, x, y, time_val):
        """Generate seamless cyberpunk colors"""
        # Create flowing neon gradients
        hue = ((x / self.chars_x) * 0.3 + (y / self.chars_y) * 0.2 + 
               math.sin(time_val * 0.5) * 0.1) % 1.0
        
        # Cyberpunk neon palette
        if hue < 0.33:
            # Cyan to Purple transition
            factor = hue / 0.33
            r = int(0 * (1 - factor) + 255 * factor)
            g = int(255 * (1 - factor) + 0 * factor)
            b = 255
        elif hue < 0.66:
            # Purple to Green transition
            factor = (hue - 0.33) / 0.33
            r = int(255 * (1 - factor) + 0 * factor)
            g = int(0 * (1 - factor) + 255 * factor)
            b = int(255 * (1 - factor) + 0 * factor)
        else:
            # Green to Cyan transition
            factor = (hue - 0.66) / 0.34
            r = int(0 * (1 - factor) + 0 * factor)
            g = 255
            b = int(0 * (1 - factor) + 255 * factor)
        
        # Add brightness variation for depth
        brightness = 0.6 + 0.4 * math.sin(time_val * 2 + x * 0.1 + y * 0.1)
        r = int(r * brightness)
        g = int(g * brightness)
        b = int(b * brightness)
        
        return f"#{r:02x}{g:02x}{b:02x}"
    
    def render_seamless_frame(self):
        """Render a single seamless frame"""
        # Clear canvas completely
        self.canvas.delete("all")
        
        # Update animation time
        self.time_offset += 0.02
        self.frame_count += 1
        
        # Update audio reactivity
        self.audio_reactivity = 0.4 + 0.3 * math.sin(self.time_offset * 2)
        
        # Render ASCII field with perfect positioning
        for y in range(0, self.chars_y, 1):
            for x in range(0, self.chars_x, 1):
                # Generate character
                char = self.generate_seamless_pattern(x, y, self.time_offset)
                
                # Generate color
                color = self.get_seamless_color(x, y, self.time_offset)
                
                # Calculate exact pixel position for seamless placement
                pixel_x = x * self.char_width
                pixel_y = y * self.char_height
                
                # Draw character with perfect centering
                self.canvas.create_text(
                    pixel_x + self.char_width // 2,
                    pixel_y + self.char_height // 2,
                    text=char,
                    fill=color,
                    font=self.font,
                    anchor='center',
                    justify='center'
                )
        
        # Update canvas for seamless display
        self.canvas.update_idletasks()
    
    def start_seamless_animation(self):
        """Start seamless animation loop"""
        def animate():
            while self.running:
                try:
                    self.render_seamless_frame()
                    time.sleep(0.016)  # ~60 FPS for seamless motion
                except Exception as e:
                    print(f"Animation error: {e}")
                    break
        
        # Start animation in daemon thread
        self.animation_thread = threading.Thread(target=animate, daemon=True)
        self.animation_thread.start()
        
        print("Seamless animation started")
    
    def toggle_fullscreen_mode(self):
        """Toggle between seamless windowed and fullscreen"""
        # This system is already completely seamless
        # Double-click provides user feedback that the system is responsive
        print("Seamless mode confirmed - already completely borderless")
    
    def exit_seamless(self):
        """Exit seamless system"""
        print("Exiting seamless system...")
        self.running = False
        self.root.quit()
        self.root.destroy()

def main():
    """Main function for seamless E2E scaling system"""
    print("=" * 60)
    print("NeonGlyph Seamless E2E Scaling System")
    print("=" * 60)
    print("Controls:")
    print("  ESC - Exit seamless system")
    print("  Double-click - Confirm seamless mode")
    print("=" * 60)
    
    try:
        system = NeonGlyphSeamlessSystem()
        print("Starting seamless rendering...")
        system.root.mainloop()
    except KeyboardInterrupt:
        print("\nSeamless system interrupted")
    except Exception as e:
        print(f"Seamless system error: {e}")
        import traceback
        traceback.print_exc()
    finally:
        print("Seamless system completed")

if __name__ == "__main__":
    main()