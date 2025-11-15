#!/usr/bin/env python3
"""
NeonGlyph Audio Reactive ASCII Visualizer Demo
This simulates what the full C++ application would show when running
"""

import numpy as np
import time
import random
import math

class AudioReactiveDemo:
    def __init__(self):
        self.bpm = 120
        self.energy = 0.5
        self.beat_phase = 0
        self.frequencies = [random.random() for _ in range(32)]
        
    def simulate_audio_analysis(self):
        """Simulate real-time audio analysis"""
        # Simulate beat detection
        self.beat_phase += (self.bpm / 60.0) * 0.1
        beat_intensity = (math.sin(self.beat_phase) + 1) / 2
        
        # Simulate frequency spectrum
        for i in range(len(self.frequencies)):
            # Simulate different frequency bands responding to music
            freq_response = math.sin(self.beat_phase * (i + 1) * 0.1) * 0.3 + 0.7
            noise = random.random() * 0.1
            self.frequencies[i] = max(0, min(1, freq_response + noise))
        
        # Simulate energy levels
        self.energy = 0.3 + beat_intensity * 0.4 + random.random() * 0.3
        self.energy = max(0, min(1, self.energy))
        
        return beat_intensity
    
    def create_ascii_visualization(self, beat_intensity):
        """Create ASCII art based on audio analysis"""
        width = 60
        height = 20
        
        # Create canvas
        canvas = [[' ' for _ in range(width)] for _ in range(height)]
        
        # Create spectrum analyzer bars
        bar_width = 2
        num_bars = width // bar_width
        
        for i in range(num_bars):
            freq_idx = int(i * len(self.frequencies) / num_bars)
            bar_height = int(self.frequencies[freq_idx] * (height - 2))
            
            # Draw bar
            for y in range(height - 2, height - 2 - bar_height, -1):
                if 0 <= y < height:
                    for x in range(i * bar_width, (i + 1) * bar_width):
                        if 0 <= x < width:
                            # Use different characters based on intensity
                            if y > height - 2 - bar_height * 0.8:
                                canvas[y][x] = '█'
                            elif y > height - 2 - bar_height * 0.5:
                                canvas[y][x] = '▓'
                            else:
                                canvas[y][x] = '▒'
        
        # Add beat detection indicator
        beat_size = int(beat_intensity * 10)
        beat_char = '●' if beat_intensity > 0.7 else '○'
        
        # Convert to string
        visualization = []
        for row in canvas:
            visualization.append(''.join(row))
        
        return '\n'.join(visualization)
    
    def create_particle_effects(self, beat_intensity):
        """Create ASCII particle effects based on audio"""
        particles = []
        
        # Create particles based on beat intensity
        num_particles = int(beat_intensity * 20)
        
        for i in range(num_particles):
            # Random position
            x = random.randint(0, 79)
            y = random.randint(0, 4)
            
            # Particle character based on energy
            if self.energy > 0.7:
                char = '★'
            elif self.energy > 0.4:
                char = '◆'
            else:
                char = '·'
            
            particles.append(f"\033[{y+1};{x+1}H{char}")
        
        return ''.join(particles)
    
    def run_demo(self):
        """Run the audio reactive demo"""
        print("🎵 NeonGlyph Audio Reactive ASCII Visualizer Demo 🎵")
        print("=" * 60)
        print("This simulates what you'd see with real audio input!")
        print("Start your YouTube video and imagine this reacting to it...")
        print("=" * 60)
        print()
        
        # Simulate different music styles
        music_styles = [
            {"name": "Electronic Beat", "bpm": 128, "energy": 0.8},
            {"name": "Rock Anthem", "bpm": 140, "energy": 0.9},
            {"name": "Chill Vibes", "bpm": 90, "energy": 0.4},
            {"name": "Pop Hit", "bpm": 118, "energy": 0.7}
        ]
        
        current_style = 0
        
        try:
            while True:
                # Switch music styles periodically
                if int(time.time()) % 10 == 0:
                    style = music_styles[current_style % len(music_styles)]
                    self.bpm = style["bpm"]
                    print(f"🎶 Now playing: {style['name']} ({style['bpm']} BPM)")
                    current_style += 1
                
                # Simulate audio analysis
                beat_intensity = self.simulate_audio_analysis()
                
                # Create visualizations
                spectrum_vis = self.create_ascii_visualization(beat_intensity)
                
                # Clear screen and display
                print("\033[2J\033[H", end='')  # Clear screen and move cursor to top
                
                print("🎵 NeonGlyph Audio Reactive System 🎵")
                print("=" * 60)
                print(f"BPM: {self.bpm}  |  Energy: {self.energy*100:.1f}%  |  Beat: {'●' if beat_intensity > 0.7 else '○'}")
                print("=" * 60)
                print()
                print("Spectrum Analysis:")
                print(spectrum_vis)
                print()
                print("Audio Characteristics:")
                print(f"• Beat Intensity: {beat_intensity*100:.1f}%")
                print(f"• Energy Level: {self.energy*100:.1f}%")
                print(f"• Frequency Response: {len([f for f in self.frequencies if f > 0.5])} active bands")
                print()
                print("This is what your YouTube audio would look like!")
                print("The real system captures actual audio via WASAPI loopback.")
                
                time.sleep(0.1)  # Update 10 times per second
                
        except KeyboardInterrupt:
            print("\n\nDemo stopped. The real NeonGlyph system would:")
            print("✅ Capture any system audio (YouTube, Spotify, etc.)")
            print("✅ Analyze it in real-time with <16ms latency")
            print("✅ Generate reactive ASCII visuals based on the music")
            print("✅ Support multiple visual themes and effects")
            print("\nTo experience the real thing, we need to resolve the build issues.")

if __name__ == "__main__":
    demo = AudioReactiveDemo()
    demo.run_demo()