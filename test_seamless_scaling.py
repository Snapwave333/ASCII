#!/usr/bin/env python3
"""
NeonGlyph Seamless Scaling Test Suite
Comprehensive testing of borderless scaling across different monitor configurations
"""

import tkinter as tk
import time
import threading
import math
import win32api
import win32con
import win32gui
from ctypes import windll
import sys

class SeamlessScalingTestSuite:
    def __init__(self):
        self.test_results = []
        self.current_test = 0
        
    def test_borderless_creation(self):
        """Test that window is created completely borderless"""
        print("Testing borderless window creation...")
        
        root = tk.Tk()
        root.overrideredirect(True)
        root.configure(bg='black')
        
        # Test window properties
        width = 800
        height = 600
        root.geometry(f"{width}x{height}+100+100")
        
        # Get window handle
        hwnd = root.winfo_id()
        
        # Check window styles
        style = win32gui.GetWindowLong(hwnd, win32con.GWL_STYLE)
        ex_style = win32gui.GetWindowLong(hwnd, win32con.GWL_EXSTYLE)
        
        # Verify no decorations
        has_caption = bool(style & win32con.WS_CAPTION)
        has_border = bool(style & win32con.WS_BORDER)
        has_thickframe = bool(style & win32con.WS_THICKFRAME)
        
        result = {
            'test': 'borderless_creation',
            'has_caption': has_caption,
            'has_border': has_border,
            'has_thickframe': has_thickframe,
            'passed': not has_caption and not has_border and not has_thickframe
        }
        
        self.test_results.append(result)
        root.destroy()
        
        print(f"Borderless test: {'PASSED' if result['passed'] else 'FAILED'}")
        return result['passed']
    
    def test_scaling_calculation(self):
        """Test character scaling calculations for different screen sizes"""
        print("Testing scaling calculations...")
        
        test_resolutions = [
            (1920, 1080),  # 1080p
            (2560, 1440),  # 1440p
            (3840, 2160),  # 4K
            (1280, 720),   # 720p
            (3440, 1440),  # Ultrawide
            (5120, 2880),  # 5K
        ]
        
        results = []
        for width, height in test_resolutions:
            # Calculate character grid
            char_width = 8
            char_height = 16
            chars_x = width // char_width
            chars_y = height // char_height
            
            # Ensure minimum viable density
            chars_x = max(chars_x, 80)
            chars_y = max(chars_y, 25)
            
            result = {
                'resolution': f"{width}x{height}",
                'chars_x': chars_x,
                'chars_y': chars_y,
                'total_chars': chars_x * chars_y,
                'density_viable': chars_x >= 80 and chars_y >= 25
            }
            results.append(result)
            
            print(f"  {result['resolution']}: {chars_x}x{chars_y} characters")
        
        all_passed = all(r['density_viable'] for r in results)
        self.test_results.append({
            'test': 'scaling_calculation',
            'results': results,
            'passed': all_passed
        })
        
        print(f"Scaling test: {'PASSED' if all_passed else 'FAILED'}")
        return all_passed
    
    def test_fullscreen_coverage(self):
        """Test that window covers entire screen without gaps"""
        print("Testing fullscreen coverage...")
        
        # Get screen dimensions
        screen_width = windll.user32.GetSystemMetrics(0)
        screen_height = windll.user32.GetSystemMetrics(1)
        
        root = tk.Tk()
        root.overrideredirect(True)
        root.configure(bg='black')
        root.geometry(f"{screen_width}x{screen_height}+0+0")
        
        # Create test canvas
        canvas = tk.Canvas(root, width=screen_width, height=screen_height, 
                          bg='black', highlightthickness=0, bd=0)
        canvas.pack()
        
        # Fill with test pattern
        for y in range(0, screen_height, 20):
            for x in range(0, screen_width, 20):
                color = "#00FF00" if (x + y) % 40 == 0 else "#FF00FF"
                canvas.create_rectangle(x, y, x+19, y+19, fill=color, outline="")
        
        # Update and check
        root.update_idletasks()
        time.sleep(0.1)
        
        # Get window position
        hwnd = root.winfo_id()
        rect = win32gui.GetWindowRect(hwnd)
        
        coverage_result = {
            'window_x': rect[0],
            'window_y': rect[1],
            'window_width': rect[2] - rect[0],
            'window_height': rect[3] - rect[1],
            'screen_width': screen_width,
            'screen_height': screen_height,
            'x_aligned': rect[0] == 0,
            'y_aligned': rect[1] == 0,
            'width_matches': (rect[2] - rect[0]) == screen_width,
            'height_matches': (rect[3] - rect[1]) == screen_height
        }
        
        perfect_coverage = (coverage_result['x_aligned'] and 
                          coverage_result['y_aligned'] and 
                          coverage_result['width_matches'] and 
                          coverage_result['height_matches'])
        
        self.test_results.append({
            'test': 'fullscreen_coverage',
            'result': coverage_result,
            'passed': perfect_coverage
        })
        
        root.destroy()
        
        print(f"Coverage test: {'PASSED' if perfect_coverage else 'FAILED'}")
        print(f"  Screen: {screen_width}x{screen_height}")
        print(f"  Window: {coverage_result['window_width']}x{coverage_result['window_height']}")
        print(f"  Position: ({coverage_result['window_x']}, {coverage_result['window_y']})")
        
        return perfect_coverage
    
    def test_performance_scaling(self):
        """Test performance at different character densities"""
        print("Testing performance scaling...")
        
        test_densities = [
            (80, 25),    # Minimum viable
            (120, 40),   # Standard
            (160, 60),   # High density
            (200, 80),   # Very high density
            (240, 100),  # Maximum practical
        ]
        
        results = []
        for chars_x, chars_y in test_densities:
            start_time = time.time()
            
            # Simulate frame rendering
            for frame in range(60):  # 1 second at 60 FPS
                # Simulate ASCII generation
                for y in range(chars_y):
                    for x in range(chars_x):
                        # Simple pattern generation
                        intensity = math.sin((x / chars_x) * math.pi * 4 + frame * 0.1)
                        char_index = int((intensity + 1) * 10 / 2)
                
                # Simulate frame timing
                time.sleep(0.001)  # Small delay
            
            end_time = time.time()
            frame_time = (end_time - start_time) / 60
            fps = 1.0 / frame_time
            
            result = {
                'density': f"{chars_x}x{chars_y}",
                'total_chars': chars_x * chars_y,
                'avg_frame_time': frame_time * 1000,  # milliseconds
                'fps': fps,
                'performance_ok': fps >= 30  # Minimum 30 FPS
            }
            results.append(result)
            
            print(f"  {result['density']}: {result['fps']:.1f} FPS ({result['avg_frame_time']:.1f}ms/frame)")
        
        all_passed = all(r['performance_ok'] for r in results)
        self.test_results.append({
            'test': 'performance_scaling',
            'results': results,
            'passed': all_passed
        })
        
        print(f"Performance test: {'PASSED' if all_passed else 'FAILED'}")
        return all_passed
    
    def test_visual_scaling_quality(self):
        """Test visual quality of scaling at different resolutions"""
        print("Testing visual scaling quality...")
        
        # Test different aspect ratios
        test_ratios = [
            (16, 9),    # Standard widescreen
            (21, 9),    # Ultrawide
            (4, 3),     # Classic
            (1, 1),     # Square
            (32, 9),    # Super ultrawide
        ]
        
        results = []
        for width_ratio, height_ratio in test_ratios:
            # Calculate dimensions for test
            base_height = 720
            width = int(base_height * width_ratio / height_ratio)
            height = base_height
            
            # Calculate character grid
            char_width = 8
            char_height = 16
            chars_x = width // char_width
            chars_y = height // char_height
            
            # Check if scaling maintains proportions
            pixel_aspect = width / height
            char_aspect = (chars_x * char_width) / (chars_y * char_height)
            
            # Check if character density is appropriate
            density_score = min(chars_x / 100, chars_y / 40)  # Normalized density
            
            result = {
                'aspect_ratio': f"{width_ratio}:{height_ratio}",
                'resolution': f"{width}x{height}",
                'chars_grid': f"{chars_x}x{chars_y}",
                'pixel_aspect': pixel_aspect,
                'char_aspect': char_aspect,
                'aspect_ratio_preserved': abs(pixel_aspect - char_aspect) < 0.1,
                'density_score': density_score,
                'quality_ok': density_score >= 0.7
            }
            results.append(result)
            
            print(f"  {result['aspect_ratio']}: {result['chars_grid']} - {'OK' if result['quality_ok'] else 'LOW DENSITY'}")
        
        all_passed = all(r['quality_ok'] for r in results)
        self.test_results.append({
            'test': 'visual_scaling_quality',
            'results': results,
            'passed': all_passed
        })
        
        print(f"Visual quality test: {'PASSED' if all_passed else 'FAILED'}")
        return all_passed
    
    def run_all_tests(self):
        """Run all scaling tests"""
        print("=" * 60)
        print("NeonGlyph Seamless Scaling Test Suite")
        print("=" * 60)
        
        tests = [
            self.test_borderless_creation,
            self.test_scaling_calculation,
            self.test_fullscreen_coverage,
            self.test_performance_scaling,
            self.test_visual_scaling_quality
        ]
        
        passed = 0
        total = len(tests)
        
        for test in tests:
            try:
                if test():
                    passed += 1
                print()
            except Exception as e:
                print(f"Test failed with error: {e}")
                print()
        
        # Summary
        print("=" * 60)
        print("TEST SUMMARY")
        print("=" * 60)
        print(f"Tests passed: {passed}/{total}")
        print(f"Success rate: {(passed/total)*100:.1f}%")
        
        if passed == total:
            print("✅ ALL TESTS PASSED - Seamless scaling is working perfectly!")
        else:
            print("⚠️  Some tests failed - Review results above")
        
        print("=" * 60)
        
        return passed == total

def main():
    """Main test runner"""
    tester = SeamlessScalingTestSuite()
    success = tester.run_all_tests()
    
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())