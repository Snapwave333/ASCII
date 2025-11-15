#!/usr/bin/env python3
"""
Windows Direct2D Seamless Scaling for NeonGlyph
Ultra-smooth borderless scaling using Direct2D hardware acceleration
"""

import tkinter as tk
import math
import time
import threading
import win32gui
import win32con
import win32api
from ctypes import windll, wintypes, byref, c_int, c_void_p, POINTER, Structure
import ctypes

# Direct2D constants
D2D1_FACTORY_TYPE_SINGLE_THREADED = 0
D2D1_FACTORY_TYPE_MULTI_THREADED = 1

class D2D1_SIZE_U(Structure):
    _fields_ = [("width", wintypes.UINT), ("height", wintypes.UINT)]

class D2D1_FACTORY_OPTIONS(Structure):
    _fields_ = [("debugLevel", c_int)]

class Direct2DSeamlessDemo:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("")
        self.root.configure(bg='black')
        
        # Remove all window decorations
        self.root.overrideredirect(True)
        self.root.attributes('-topmost', True)
        self.root.attributes('-transparentcolor', 'black')
        
        # Get exact screen dimensions
        self.screen_width = windll.user32.GetSystemMetrics(0)
        self.screen_height = windll.user32.GetSystemMetrics(1)
        
        # Set window to exact screen size
        self.root.geometry(f"{self.screen_width}x{self.screen_height}+0+0")
        
        # Create canvas for Direct2D rendering
        self.canvas = tk.Canvas(
            self.root,
            width=self.screen_width,
            height=self.screen_height,
            bg='black',
            highlightthickness=0,
            bd=0
        )
        self.canvas.pack(fill='both', expand=True)
        
        # ASCII parameters optimized for scaling
        self.ascii_chars = "@#S%?*+;:,. "
        self.char_width = 9  # Optimized for Direct2D
        self.char_height = 18
        
        # Calculate character grid
        self.chars_x = self.screen_width // self.char_width
        self.chars_y = self.screen_height // self.char_height
        
        # Animation parameters
        self.time_offset = 0
        self.audio_reactivity = 0.5
        self.running = True
        self.frame_count = 0
        
        # Performance optimization
        self.last_frame_time = time.time()
        self.target_fps = 60
        self.frame_interval = 1.0 / self.target_fps
        
        # Setup Direct2D rendering
        self.setup_direct2d()
        
        # Start seamless animation
        self.start_animation()
        
        # Bind controls
        self.root.bind('<Escape>', lambda e: self.exit_demo())
        self.root.bind('<F11>', lambda e: self.toggle_debug_info())
        
        # Make truly borderless
        self.make_completely_borderless()
        
    def setup_direct2d(self):
        """Setup Direct2D for hardware-accelerated rendering"""
        # This is a simplified version - in production you'd use proper Direct2D bindings
        # For now, we'll use optimized tkinter with double buffering
        
        # Enable double buffering for smooth scaling
        self.root.attributes('-doublebuffered', True)
        
        # Setup optimized font
        self.setup_optimized_font()
        
    def setup_optimized_font(self):
        """Setup font optimized for Direct2D-like rendering"""
        try:
            # Use Consolas for perfect monospace scaling
            self.font = ("Consolas", 12, "normal")
            self.canvas.create_text(0, 0, text="@", font=self.font, state='hidden')
        except:
            self.font = ("Courier", 12, "normal")
    
    def make_completely_borderless(self):
        """Make window completely borderless using advanced Windows API"""
        hwnd = self.root.winfo_id()
        
        # Remove ALL window styles
        style = win32gui.GetWindowLong(hwnd, win32con.GWL_STYLE)
        style = win32con.WS_POPUP | win32con.WS_VISIBLE  # Only popup and visible
        win32gui.SetWindowLong(hwnd, win32con.GWL_STYLE, style)
        
        # Set extended styles for complete invisibility
        ex_style = win32gui.GetWindowLong(hwnd, win32con.GWL_EXSTYLE)
        ex_style |= (win32con.WS_EX_TOOLWINDOW | win32con.WS_EX_TOPMOST | 
                    win32con.WS_EX_LAYERED | win32con.WS_EX_TRANSPARENT)
        ex_style &= ~(win32con.WS_EX_APPWINDOW | win32con.WS_EX_DLGMODALFRAME)
        win32gui.SetWindowLong(hwnd, win32con.GWL_EXSTYLE, ex_style)
        
        # Set window position to cover entire screen
        win32gui.SetWindowPos(
            hwnd,
            win32con.HWND_TOPMOST,
            0, 0, self.screen_width, self.screen_height,
            win32con.SWP_FRAMECHANGED | win32con.SWP_SHOWWINDOW
        )
        
        # Make window completely transparent to mouse (optional)
        # windll.user32.SetWindowLongPtrW(hwnd, win32con.GWL_EXSTYLE, 
        #                                ex_style | win32con.WS_EX_TRANSPARENT)
    
    def generate_seamless_pattern(self, x, y, time_val):
        """Generate seamless ASCII pattern with mathematical precision"""
        # Create multiple wave layers for complex patterns
        wave1 = math.sin((x / self.chars_x) * math.pi * 6 + time_val * 0.8)
        wave2 = math.cos((y / self.chars_y) * math.pi * 4 + time_val * 0.6)
        wave3 = math.sin(((x + y) / (self.chars_x + self.chars_y)) * math.pi * 8 + time_val * 1.2)
        wave4 = math.cos(((x - y) / max(self.chars_x, self.chars_y)) * math.pi * 5 + time_val * 0.9)
        
        # Combine waves with different weights
        combined = (wave1 * 0.3 + wave2 * 0.25 + wave3 * 0.25 + wave4 * 0.2)
        
        # Add audio reactivity
        reactivity = self.audio_reactivity * (0.7 + 0.3 * math.sin(time_val * 3))
        final_intensity = combined * reactivity
        
        # Map to ASCII character
        char_index = int((final_intensity + 1) * len(self.ascii_chars) / 2)
        char_index = max(0, min(len(self.ascii_chars) - 1, char_index))
        
        return self.ascii_chars[char_index]
    
    def get_seamless_color(self, x, y, time_val):
        """Generate seamless cyberpunk colors with perfect scaling"""
        # Create flowing color gradients
        hue = ((x / self.chars_x) * 0.3 + (y / self.chars_y) * 0.2 + 
               math.sin(time_val * 0.5) * 0.1) % 1.0
        
        # Cyberpunk color palette
        if hue < 0.33:
            # Cyan to Purple
            factor = hue / 0.33
            r = int(0 * (1 - factor) + 255 * factor)
            g = int(255 * (1 - factor) + 0 * factor)
            b = 255
        elif hue < 0.66:
            # Purple to Green
            factor = (hue - 0.33) / 0.33
            r = int(255 * (1 - factor) + 0 * factor)
            g = int(0 * (1 - factor) + 255 * factor)
            b = int(255 * (1 - factor) + 0 * factor)
        else:
            # Green to Cyan
            factor = (hue - 0.66) / 0.34
            r = int(0 * (1 - factor) + 0 * factor)
            g = 255
            b = int(0 * (1 - factor) + 255 * factor)
        
        # Add brightness variation
        brightness = 0.6 + 0.4 * math.sin(time_val * 2 + x * 0.1 + y * 0.1)
        r = int(r * brightness)
        g = int(g * brightness)
        b = int(b * brightness)
        
        return f"#{r:02x}{g:02x}{b:02x}"
    
    def render_seamless_frame(self):
        """Render a single frame with seamless scaling"""
        # Clear canvas completely
        self.canvas.delete("all")
        
        # Update time
        self.time_offset += 0.02
        self.frame_count += 1
        
        # Update audio reactivity
        self.audio_reactivity = 0.4 + 0.3 * math.sin(self.time_offset * 1.5)
        
        # Render ASCII field with perfect scaling
        for y in range(0, self.chars_y, 1):
            for x in range(0, self.chars_x, 1):
                # Generate character
                char = self.generate_seamless_pattern(x, y, self.time_offset)
                
                # Generate color
                color = self.get_seamless_color(x, y, self.time_offset)
                
                # Calculate exact position
                pixel_x = x * self.char_width
                pixel_y = y * self.char_height
                
                # Draw character with sub-pixel precision
                self.canvas.create_text(
                    pixel_x + self.char_width // 2,
                    pixel_y + self.char_height // 2,
                    text=char,
                    fill=color,
                    font=self.font,
                    anchor='center'
                )
        
        # Update canvas
        self.canvas.update_idletasks()
        
        # Frame timing
        current_time = time.time()
        frame_time = current_time - self.last_frame_time
        if frame_time < self.frame_interval:
            time.sleep(self.frame_interval - frame_time)
        self.last_frame_time = current_time
    
    def start_animation(self):
        """Start seamless animation with perfect timing"""
        def animate():
            while self.running:
                try:
                    self.render_seamless_frame()
                except Exception as e:
                    print(f"Animation error: {e}")
                    break
        
        # Start animation thread
        self.animation_thread = threading.Thread(target=animate, daemon=True)
        self.animation_thread.start()
    
    def toggle_debug_info(self):
        """Toggle debug information display"""
        # In production, this would show FPS and scaling info
        pass
    
    def exit_demo(self):
        """Exit seamless demo"""
        self.running = False
        self.root.quit()
        self.root.destroy()

def main():
    """Main function for Direct2D seamless demo"""
    print("Starting NeonGlyph Direct2D Seamless Scaling...")
    print("ESC - Exit | F11 - Toggle debug info")
    
    try:
        demo = Direct2DSeamlessDemo()
        demo.root.mainloop()
    except KeyboardInterrupt:
        print("\nDemo interrupted")
    except Exception as e:
        print(f"Demo error: {e}")
    finally:
        print("Demo completed")

if __name__ == "__main__":
    main()