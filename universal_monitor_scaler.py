#!/usr/bin/env python3
"""
Universal Monitor Scaling System for NeonGlyph
Automatically detects and adapts to any monitor configuration
"""

import tkinter as tk
import math
import time
import threading
import win32api
import win32con
import win32gui
import win32ui
from ctypes import windll, wintypes, Structure, POINTER, c_int, c_uint, c_void_p
import ctypes

# Monitor enumeration structures
class RECT(Structure):
    _fields_ = [("left", c_int), ("top", c_int), ("right", c_int), ("bottom", c_int)]

class MONITORINFO(Structure):
    _fields_ = [
        ("cbSize", c_uint),
        ("rcMonitor", RECT),
        ("rcWork", RECT),
        ("dwFlags", c_uint)
    ]

class UniversalMonitorScaler:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("")
        self.root.configure(bg='black')
        
        # Remove all decorations
        self.root.overrideredirect(True)
        self.root.attributes('-topmost', True)
        self.root.attributes('-transparentcolor', 'black')
        
        # Detect all monitors
        self.monitors = self.detect_all_monitors()
        self.primary_monitor = self.get_primary_monitor()
        
        # Use primary monitor for main display
        self.current_monitor = self.primary_monitor
        self.setup_for_monitor(self.current_monitor)
        
        # Create canvas
        self.canvas = tk.Canvas(
            self.root,
            width=self.current_monitor['width'],
            height=self.current_monitor['height'],
            bg='black',
            highlightthickness=0,
            bd=0
        )
        self.canvas.pack(fill='both', expand=True)
        
        # Scaling parameters
        self.ascii_chars = "@#S%?*+;:,. "
        self.base_char_width = 8
        self.base_char_height = 16
        
        # Calculate optimal character size for current monitor
        self.calculate_optimal_character_size()
        
        # Animation
        self.time_offset = 0
        self.audio_reactivity = 0.5
        self.running = True
        
        # Performance
        self.target_fps = 60
        self.frame_interval = 1.0 / self.target_fps
        self.last_frame_time = time.time()
        
        # Setup
        self.setup_optimized_font()
        self.start_animation()
        
        # Controls
        self.root.bind('<Escape>', lambda e: self.exit_demo())
        self.root.bind('<F1>', lambda e: self.switch_to_next_monitor())
        self.root.bind('<F2>', lambda e: self.recalculate_scaling())
        
        # Make completely borderless
        self.make_universally_borderless()
        
    def detect_all_monitors(self):
        """Detect all connected monitors with detailed information"""
        monitors = []
        
        def enum_proc(hMonitor, hdcMonitor, lprcMonitor, dwData):
            monitor_info = MONITORINFO()
            monitor_info.cbSize = ctypes.sizeof(MONITORINFO)
            
            if windll.user32.GetMonitorInfoW(hMonitor, ctypes.byref(monitor_info)):
                width = monitor_info.rcMonitor.right - monitor_info.rcMonitor.left
                height = monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top
                
                monitor_data = {
                    'handle': hMonitor,
                    'width': width,
                    'height': height,
                    'aspect_ratio': width / height,
                    'is_primary': bool(monitor_info.dwFlags & 1),  # MONITORINFOF_PRIMARY
                    'position': {
                        'x': monitor_info.rcMonitor.left,
                        'y': monitor_info.rcMonitor.top
                    },
                    'work_area': {
                        'left': monitor_info.rcWork.left,
                        'top': monitor_info.rcWork.top,
                        'right': monitor_info.rcWork.right,
                        'bottom': monitor_info.rcWork.bottom
                    }
                }
                
                monitors.append(monitor_data)
            
            return True
        
        # EnumDisplayMonitors callback
        MONITORENUMPROC = ctypes.WINFUNCTYPE(c_int, c_void_p, c_void_p, POINTER(RECT), c_int)
        callback = MONITORENUMPROC(enum_proc)
        
        windll.user32.EnumDisplayMonitors(None, None, callback, 0)
        
        return monitors
    
    def get_primary_monitor(self):
        """Get the primary monitor"""
        for monitor in self.monitors:
            if monitor['is_primary']:
                return monitor
        return self.monitors[0] if self.monitors else None
    
    def setup_for_monitor(self, monitor):
        """Setup window for specific monitor"""
        self.current_monitor = monitor
        self.screen_width = monitor['width']
        self.screen_height = monitor['height']
        
        # Position window on correct monitor
        x_pos = monitor['position']['x']
        y_pos = monitor['position']['y']
        
        self.root.geometry(f"{self.screen_width}x{self.screen_height}+{x_pos}+{y_pos}")
        
        # Recalculate character size for this monitor
        self.calculate_optimal_character_size()
    
    def calculate_optimal_character_size(self):
        """Calculate optimal character size for current monitor"""
        # Base calculations
        self.chars_x = self.screen_width // self.base_char_width
        self.chars_y = self.screen_height // self.base_char_height
        
        # Adjust for monitor DPI and size
        dpi_scale = self.get_monitor_dpi_scale()
        
        # Optimize character density based on monitor size
        if self.screen_width >= 3840:  # 4K
            density_factor = 1.2
        elif self.screen_width >= 2560:  # 1440p
            density_factor = 1.0
        elif self.screen_width >= 1920:  # 1080p
            density_factor = 0.9
        else:  # Lower resolutions
            density_factor = 0.8
        
        # Apply scaling
        self.chars_x = int(self.chars_x * density_factor * dpi_scale)
        self.chars_y = int(self.chars_y * density_factor * dpi_scale)
        
        # Ensure minimum viable character count
        self.chars_x = max(80, self.chars_x)
        self.chars_y = max(25, self.chars_y)
        
        # Recalculate actual character size
        self.actual_char_width = self.screen_width // self.chars_x
        self.actual_char_height = self.screen_height // self.chars_y
    
    def get_monitor_dpi_scale(self):
        """Get DPI scaling factor for current monitor"""
        try:
            # Get DPI for primary monitor
            hdc = windll.user32.GetDC(None)
            dpi = windll.gdi32.GetDeviceCaps(hdc, 88)  # LOGPIXELSX
            windll.user32.ReleaseDC(None, hdc)
            
            # Standard DPI is 96
            return dpi / 96.0
        except:
            return 1.0
    
    def setup_optimized_font(self):
        """Setup font optimized for current monitor"""
        # Calculate optimal font size
        font_size = max(8, min(16, self.actual_char_height - 2))
        
        try:
            self.font = ("Consolas", font_size, "normal")
            self.canvas.create_text(0, 0, text="@", font=self.font, state='hidden')
        except:
            self.font = ("Courier", font_size, "normal")
    
    def make_universally_borderless(self):
        """Make window completely borderless on any monitor"""
        hwnd = self.root.winfo_id()
        
        # Remove ALL window decorations
        style = win32gui.GetWindowLong(hwnd, win32con.GWL_STYLE)
        style = win32con.WS_POPUP | win32con.WS_VISIBLE
        win32gui.SetWindowLong(hwnd, win32con.GWL_STYLE, style)
        
        # Set extended styles for universal compatibility
        ex_style = win32gui.GetWindowLong(hwnd, win32con.GWL_EXSTYLE)
        ex_style |= (win32con.WS_EX_TOOLWINDOW | win32con.WS_EX_TOPMOST | 
                    win32con.WS_EX_LAYERED | win32con.WS_EX_TRANSPARENT)
        ex_style &= ~(win32con.WS_EX_APPWINDOW | win32con.WS_EX_DLGMODALFRAME)
        win32gui.SetWindowLong(hwnd, win32con.GWL_EXSTYLE, ex_style)
        
        # Set window position and size
        win32gui.SetWindowPos(
            hwnd,
            win32con.HWND_TOPMOST,
            self.current_monitor['position']['x'],
            self.current_monitor['position']['y'],
            self.screen_width,
            self.screen_height,
            win32con.SWP_FRAMECHANGED | win32con.SWP_SHOWWINDOW
        )
    
    def generate_universal_pattern(self, x, y, time_val):
        """Generate pattern that scales perfectly on any monitor"""
        # Create multi-layer wave patterns
        wave1 = math.sin((x / self.chars_x) * math.pi * 8 + time_val * 0.7)
        wave2 = math.cos((y / self.chars_y) * math.pi * 6 + time_val * 0.5)
        wave3 = math.sin(((x + y) / (self.chars_x + self.chars_y)) * math.pi * 10 + time_val * 1.0)
        wave4 = math.cos(((x - y) / max(self.chars_x, self.chars_y)) * math.pi * 7 + time_val * 0.8)
        
        # Combine with monitor-specific scaling
        combined = (wave1 * 0.25 + wave2 * 0.25 + wave3 * 0.25 + wave4 * 0.25)
        
        # Add audio reactivity
        reactivity = self.audio_reactivity * (0.6 + 0.4 * math.sin(time_val * 2.5))
        final_intensity = combined * reactivity
        
        # Map to ASCII
        char_index = int((final_intensity + 1) * len(self.ascii_chars) / 2)
        char_index = max(0, min(len(self.ascii_chars) - 1, char_index))
        
        return self.ascii_chars[char_index]
    
    def get_universal_color(self, x, y, time_val):
        """Generate colors that look perfect on any monitor"""
        # Create flowing gradients
        hue = ((x / self.chars_x) * 0.4 + (y / self.chars_y) * 0.3 + 
               math.sin(time_val * 0.6) * 0.15) % 1.0
        
        # Cyberpunk palette
        if hue < 0.25:
            # Cyan zone
            factor = hue / 0.25
            r = int(0 * (1 - factor) + 0 * factor)
            g = int(255 * (1 - factor) + 100 * factor)
            b = int(255 * (1 - factor) + 255 * factor)
        elif hue < 0.5:
            # Purple zone
            factor = (hue - 0.25) / 0.25
            r = int(0 * (1 - factor) + 255 * factor)
            g = int(100 * (1 - factor) + 0 * factor)
            b = int(255 * (1 - factor) + 255 * factor)
        elif hue < 0.75:
            # Green zone
            factor = (hue - 0.5) / 0.25
            r = int(255 * (1 - factor) + 0 * factor)
            g = int(0 * (1 - factor) + 255 * factor)
            b = int(255 * (1 - factor) + 0 * factor)
        else:
            # Back to Cyan
            factor = (hue - 0.75) / 0.25
            r = int(0 * (1 - factor) + 0 * factor)
            g = int(255 * (1 - factor) + 255 * factor)
            b = int(0 * (1 - factor) + 255 * factor)
        
        # Add brightness variation
        brightness = 0.5 + 0.5 * math.sin(time_val * 1.5 + x * 0.05 + y * 0.05)
        r = int(r * brightness)
        g = int(g * brightness)
        b = int(b * brightness)
        
        return f"#{r:02x}{g:02x}{b:02x}"
    
    def render_universal_frame(self):
        """Render frame with universal scaling"""
        # Clear canvas
        self.canvas.delete("all")
        
        # Update time
        self.time_offset += 0.016  # ~60 FPS timing
        
        # Update audio reactivity
        self.audio_reactivity = 0.4 + 0.3 * math.sin(self.time_offset * 2)
        
        # Render ASCII field
        for y in range(0, self.chars_y, 1):
            for x in range(0, self.chars_x, 1):
                char = self.generate_universal_pattern(x, y, self.time_offset)
                color = self.get_universal_color(x, y, self.time_offset)
                
                pixel_x = x * self.actual_char_width
                pixel_y = y * self.actual_char_height
                
                self.canvas.create_text(
                    pixel_x + self.actual_char_width // 2,
                    pixel_y + self.actual_char_height // 2,
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
        """Start universal animation"""
        def animate():
            while self.running:
                try:
                    self.render_universal_frame()
                except Exception as e:
                    print(f"Animation error: {e}")
                    break
        
        self.animation_thread = threading.Thread(target=animate, daemon=True)
        self.animation_thread.start()
    
    def switch_to_next_monitor(self):
        """Switch to next available monitor"""
        current_index = self.monitors.index(self.current_monitor)
        next_index = (current_index + 1) % len(self.monitors)
        next_monitor = self.monitors[next_index]
        
        print(f"Switching to monitor: {next_monitor['width']}x{next_monitor['height']}")
        self.setup_for_monitor(next_monitor)
        
        # Update canvas size
        self.canvas.config(width=self.screen_width, height=self.screen_height)
        self.setup_optimized_font()
        self.make_universally_borderless()
    
    def recalculate_scaling(self):
        """Recalculate optimal scaling for current monitor"""
        print("Recalculating optimal scaling...")
        self.calculate_optimal_character_size()
        self.setup_optimized_font()
        print(f"New character grid: {self.chars_x}x{self.chars_y}")
    
    def exit_demo(self):
        """Exit universal demo"""
        self.running = False
        self.root.quit()
        self.root.destroy()

def main():
    """Main function for universal monitor scaling"""
    print("Starting NeonGlyph Universal Monitor Scaling...")
    print("Controls:")
    print("  ESC - Exit")
    print("  F1 - Switch to next monitor")
    print("  F2 - Recalculate scaling")
    
    try:
        scaler = UniversalMonitorScaler()
        
        # Print monitor information
        print(f"\nDetected {len(scaler.monitors)} monitor(s):")
        for i, monitor in enumerate(scaler.monitors):
            primary = " (PRIMARY)" if monitor['is_primary'] else ""
            print(f"  Monitor {i+1}: {monitor['width']}x{monitor['height']}{primary}")
        
        scaler.root.mainloop()
    except KeyboardInterrupt:
        print("\nDemo interrupted")
    except Exception as e:
        print(f"Demo error: {e}")
    finally:
        print("Demo completed")

if __name__ == "__main__":
    main()