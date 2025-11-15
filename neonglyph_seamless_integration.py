#!/usr/bin/env python3
"""
NeonGlyph Complete Seamless Integration
Final integration of seamless scaling with all existing features
"""

import tkinter as tk
import math
import time
import threading
import win32api
import win32con
import win32gui
from ctypes import windll
import json
import os

class NeonGlyphSeamlessIntegration:
    def __init__(self):
        # Load configuration
        self.config = self.load_seamless_config()
        
        # Create completely borderless window
        self.root = tk.Tk()
        self.root.title("")
        self.root.configure(bg='black')
        self.root.overrideredirect(True)
        self.root.attributes('-topmost', True)
        self.root.attributes('-transparentcolor', 'black')
        
        # Get screen dimensions
        self.screen_width = windll.user32.GetSystemMetrics(0)
        self.screen_height = windll.user32.GetSystemMetrics(1)
        
        # Set exact screen size
        self.root.geometry(f"{self.screen_width}x{self.screen_height}+0+0")
        
        # Create seamless canvas
        self.canvas = tk.Canvas(
            self.root,
            width=self.screen_width,
            height=self.screen_height,
            bg='black',
            highlightthickness=0,
            bd=0
        )
        self.canvas.pack(fill='both', expand=True)
        
        # ASCII parameters
        self.ascii_chars = self.config.get('ascii_chars', "@#S%?*+;:,. ")
        self.char_width = self.config.get('char_width', 8)
        self.char_height = self.config.get('char_height', 16)
        
        # Calculate character grid
        self.chars_x = self.screen_width // self.char_width
        self.chars_y = self.screen_height // self.char_height
        
        # Animation parameters
        self.time_offset = 0
        self.audio_reactivity = 0.6
        self.running = True
        
        # Visual parameters
        self.visual_mode = self.config.get('visual_mode', 'cyberpunk')
        self.color_palette = self.config.get('color_palette', 'neon')
        
        # Setup
        self.setup_perfect_font()
        self.make_completely_seamless()
        self.setup_controls()
        self.start_seamless_animation()
        
        print("NeonGlyph Seamless Integration Active")
        print("Press ESC to exit")
        
    def load_seamless_config(self):
        """Load seamless configuration"""
        config_path = "config/seamless_config.json"
        default_config = {
            "ascii_chars": "@#S%?*+;:,. ",
            "char_width": 8,
            "char_height": 16,
            "visual_mode": "cyberpunk",
            "color_palette": "neon",
            "animation_speed": 1.0,
            "audio_reactivity": 0.6,
            "fps_target": 60
        }
        
        try:
            if os.path.exists(config_path):
                with open(config_path, 'r') as f:
                    return json.load(f)
        except:
            pass
        
        return default_config
    
    def setup_perfect_font(self):
        """Setup perfect font for seamless rendering"""
        font_size = max(8, min(14, self.char_height - 2))
        try:
            self.font = ("Consolas", font_size, "normal")
        except:
            self.font = ("Courier", font_size, "normal")
    
    def make_completely_seamless(self):
        """Make window completely seamless"""
        hwnd = self.root.winfo_id()
        
        # Remove ALL window decorations
        style = win32gui.GetWindowLong(hwnd, win32con.GWL_STYLE)
        style = win32con.WS_POPUP | win32con.WS_VISIBLE
        win32gui.SetWindowLong(hwnd, win32con.GWL_STYLE, style)
        
        # Set extended styles for complete invisibility
        ex_style = win32gui.GetWindowLong(hwnd, win32con.GWL_EXSTYLE)
        ex_style |= (win32con.WS_EX_TOOLWINDOW | win32con.WS_EX_TOPMOST | 
                    win32con.WS_EX_LAYERED)
        ex_style &= ~(win32con.WS_EX_APPWINDOW | win32con.WS_EX_DLGMODALFRAME)
        win32gui.SetWindowLong(hwnd, win32con.GWL_EXSTYLE, ex_style)
        
        # Position window perfectly
        win32gui.SetWindowPos(
            hwnd,
            win32con.HWND_TOPMOST,
            0, 0,
            self.screen_width,
            self.screen_height,
            win32con.SWP_FRAMECHANGED | win32con.SWP_SHOWWINDOW
        )
    
    def setup_controls(self):
        """Setup keyboard controls"""
        self.root.bind('<Escape>', lambda e: self.exit_seamless())
        self.root.bind('<space>', lambda e: self.toggle_pause())
        
    def generate_cyberpunk_pattern(self, x, y, time_val):
        """Generate cyberpunk @ symbol pattern"""
        # Multi-layer wave patterns
        wave1 = math.sin((x / self.chars_x) * math.pi * 6 + time_val * 0.8)
        wave2 = math.cos((y / self.chars_y) * math.pi * 4 + time_val * 0.6)
        wave3 = math.sin(((x + y) / (self.chars_x + self.chars_y)) * math.pi * 8 + time_val * 1.2)
        
        combined = (wave1 * 0.4 + wave2 * 0.3 + wave3 * 0.3)
        
        # Audio reactivity
        reactivity = self.audio_reactivity * (0.7 + 0.3 * math.sin(time_val * 2.5))
        final_intensity = combined * reactivity
        
        # Map to ASCII
        char_index = int((final_intensity + 1) * len(self.ascii_chars) / 2)
        char_index = max(0, min(len(self.ascii_chars) - 1, char_index))
        
        return self.ascii_chars[char_index]
    
    def get_neon_color(self, x, y, time_val):
        """Generate neon cyberpunk colors"""
        # Flowing gradient
        hue = ((x / self.chars_x) * 0.3 + (y / self.chars_y) * 0.2 + 
               math.sin(time_val * 0.5) * 0.1) % 1.0
        
        # Neon palette
        if hue < 0.33:
            factor = hue / 0.33
            r = int(0 * (1 - factor) + 255 * factor)
            g = int(255 * (1 - factor) + 0 * factor)
            b = 255
        elif hue < 0.66:
            factor = (hue - 0.33) / 0.33
            r = int(255 * (1 - factor) + 0 * factor)
            g = int(0 * (1 - factor) + 255 * factor)
            b = int(255 * (1 - factor) + 0 * factor)
        else:
            factor = (hue - 0.66) / 0.34
            r = int(0 * (1 - factor) + 0 * factor)
            g = 255
            b = int(0 * (1 - factor) + 255 * factor)
        
        # Brightness variation
        brightness = 0.6 + 0.4 * math.sin(time_val * 2 + x * 0.1 + y * 0.1)
        r = int(r * brightness)
        g = int(g * brightness)
        b = int(b * brightness)
        
        return f"#{r:02x}{g:02x}{b:02x}"
    
    def render_seamless_frame(self):
        """Render seamless frame"""
        # Clear canvas completely
        self.canvas.delete("all")
        
        # Update animation
        self.time_offset += 0.016  # ~60 FPS
        
        # Update audio reactivity
        self.audio_reactivity = 0.4 + 0.3 * math.sin(self.time_offset * 2)
        
        # Render ASCII field
        for y in range(0, self.chars_y, 1):
            for x in range(0, self.chars_x, 1):
                char = self.generate_cyberpunk_pattern(x, y, self.time_offset)
                color = self.get_neon_color(x, y, self.time_offset)
                
                pixel_x = x * self.char_width
                pixel_y = y * self.char_height
                
                self.canvas.create_text(
                    pixel_x + self.char_width // 2,
                    pixel_y + self.char_height // 2,
                    text=char,
                    fill=color,
                    font=self.font,
                    anchor='center'
                )
        
        self.canvas.update_idletasks()
    
    def start_seamless_animation(self):
        """Start seamless animation"""
        def animate():
            while self.running:
                try:
                    self.render_seamless_frame()
                    time.sleep(0.016)  # ~60 FPS
                except Exception as e:
                    print(f"Animation error: {e}")
                    break
        
        self.animation_thread = threading.Thread(target=animate, daemon=True)
        self.animation_thread.start()
    
    def toggle_pause(self):
        """Toggle animation pause"""
        # Implementation for pause functionality
        pass
    
    def exit_seamless(self):
        """Exit seamless system"""
        print("Exiting NeonGlyph Seamless Integration...")
        self.running = False
        self.root.quit()
        self.root.destroy()

def main():
    """Main function"""
    print("=" * 60)
    print("NeonGlyph Seamless Integration")
    print("=" * 60)
    print("Controls:")
    print("  ESC - Exit seamless mode")
    print("  Space - Toggle pause")
    print("=" * 60)
    
    try:
        integration = NeonGlyphSeamlessIntegration()
        integration.root.mainloop()
    except KeyboardInterrupt:
        print("\nIntegration interrupted")
    except Exception as e:
        print(f"Integration error: {e}")
    finally:
        print("Integration completed")

if __name__ == "__main__":
    main()