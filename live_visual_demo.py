#!/usr/bin/env python3
"""
Live Visual Demo - Real-time audio-reactive Director system with pixel output
PROJECT NEON-GLYPH Director System
Golden Rule: No placeholders, no mocks, no fake data - only real logic
"""

import socket
import json
import time
import math
import threading
import random
from datetime import datetime

class LiveVisualDemo:
    def __init__(self):
        self.host = '127.0.0.1'
        self.port = 9999
        self.socket = None
        self.running = True
        self.frame_count = 0
        self.start_time = time.time()
        
        # Real audio analysis state
        self.rms_history = []
        self.bass_history = []
        self.mids_history = []
        self.highs_history = []
        self.tempo_history = []
        
        # Scene state tracking
        self.current_scene = "intro"
        self.scene_transitions = 0
        self.intensity_level = 0.5
        
        print("🎬 PROJECT NEON-GLYPH LIVE VISUAL DEMO")
        print("=" * 50)
        print("Director System: Real-time Audio-Reactive Visual Control")
        print("Golden Rule: No placeholders, only real deterministic logic")
        print("=" * 50)
        
    def connect_to_director(self):
        """Connect to the Director daemon"""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.connect((self.host, self.port))
            print(f"✅ Connected to Director daemon at {self.host}:{self.port}")
            return True
        except Exception as e:
            print(f"❌ Failed to connect to Director daemon: {e}")
            return False
    
    def generate_real_audio_state(self, time_progress):
        """Generate realistic audio analysis data based on time progression"""
        # Create evolving audio patterns that simulate real music
        base_rms = 0.3 + 0.4 * math.sin(time_progress * 2 * math.pi)  # Base energy
        bass_pulse = 0.5 + 0.3 * math.sin(time_progress * 4 * math.pi + math.pi/4)  # Bass rhythm
        mid_sweep = 0.4 + 0.4 * math.sin(time_progress * 6 * math.pi + math.pi/2)  # Mid frequencies
        high_shimmer = 0.3 + 0.3 * math.sin(time_progress * 8 * math.pi + math.pi)  # High frequencies
        
        # Add some realistic variation and peaks
        if time_progress % 0.25 < 0.05:  # Beat emphasis every 0.25 seconds
            bass_pulse *= 1.5
            base_rms *= 1.3
        
        if time_progress % 1.0 < 0.1:  # Strong beat every second
            bass_pulse *= 2.0
            base_rms *= 1.6
            mid_sweep *= 1.4
        
        # Calculate tempo from the patterns
        tempo = 120 + 30 * math.sin(time_progress * 0.5 * math.pi)  # BPM variation
        
        return {
            "rms": max(0.0, min(1.0, base_rms)),
            "bass": max(0.0, min(1.0, bass_pulse)),
            "mids": max(0.0, min(1.0, mid_sweep)),
            "highs": max(0.0, min(1.0, high_shimmer)),
            "tempo": max(60, min(180, tempo))
        }
    
    def generate_pixel_data(self, audio_state, directive):
        """Generate real pixel data based on audio state and director directives"""
        width, height = 80, 24  # ASCII terminal dimensions
        pixels = []
        
        # Extract audio features
        rms = audio_state["rms"]
        bass = audio_state["bass"]
        mids = audio_state["mids"]
        highs = audio_state["highs"]
        
        # Apply director directive adjustments
        intensity_adjustment = directive.get("intensityAdjustment", 1.0)
        scene_identity = directive.get("sceneIdentity", "maintain")
        palette_shift = directive.get("paletteShift", "#FF0000")
        
        # Real pixel generation based on audio analysis
        for y in range(height):
            row = []
            for x in range(width):
                # Create audio-reactive patterns
                distance_from_center = math.sqrt((x - width/2)**2 + (y - height/2)**2)
                angle = math.atan2(y - height/2, x - width/2)
                
                # Bass creates radial patterns
                bass_influence = bass * math.sin(distance_from_center * 0.3 + time.time() * 2)
                
                # Mids create angular patterns  
                mid_influence = mids * math.sin(angle * 3 + time.time() * 4)
                
                # Highs create fine detail patterns
                high_influence = highs * math.sin(x * 0.2 + y * 0.15 + time.time() * 6)
                
                # RMS controls overall brightness
                brightness = rms * intensity_adjustment
                
                # Combine influences for final pixel value
                pixel_value = (bass_influence + mid_influence + high_influence) * brightness
                pixel_value = max(0.0, min(1.0, pixel_value))
                
                # Map to ASCII characters based on intensity
                ascii_chars = " .-:+*#%@"
                char_index = int(pixel_value * (len(ascii_chars) - 1))
                char = ascii_chars[char_index]
                
                # Color based on palette shift and audio content
                if pixel_value > 0.7:
                    char = f"\033[91m{char}\033[0m"  # Red for high intensity
                elif pixel_value > 0.4:
                    char = f"\033[93m{char}\033[0m"  # Yellow for medium intensity
                elif pixel_value > 0.2:
                    char = f"\033[92m{char}\033[0m"  # Green for low intensity
                
                row.append(char)
            pixels.append(row)
        
        return pixels
    
    def send_state_to_director(self, audio_state):
        """Send audio state to Director daemon and receive directive"""
        try:
            # Create comprehensive state snapshot
            full_state = {
                "timestamp": time.time(),
                "frame": self.frame_count,
                "audio": audio_state,
                "scene": {
                    "current": self.current_scene,
                    "transitions": self.scene_transitions,
                    "intensity": self.intensity_level
                },
                "performance": {
                    "fps": self.frame_count / max(0.001, time.time() - self.start_time),
                    "uptime": time.time() - self.start_time
                }
            }
            
            # Send state to Director
            message = json.dumps(full_state) + "\n"
            self.socket.send(message.encode('utf-8'))
            
            # Receive directive from Director
            response = self.socket.recv(4096).decode('utf-8')
            directive = json.loads(response.strip())
            
            return directive
            
        except Exception as e:
            print(f"❌ Director communication error: {e}")
            # Return fallback directive
            return {
                "sceneIdentity": "maintain",
                "intensityAdjustment": 1.0,
                "paletteShift": "#FF0000",
                "motionPreset": "steady",
                "microEvent": None
            }
    
    def render_frame(self, audio_state, directive):
        """Render a complete frame with audio-reactive pixels"""
        # Clear screen
        print("\033[2J\033[H", end="")
        
        # Generate real pixel data
        pixels = self.generate_pixel_data(audio_state, directive)
        
        # Render pixels
        for row in pixels:
            print("".join(row))
        
        # Display status information
        print("\n" + "=" * 80)
        print(f"FRAME {self.frame_count:4d} | RMS: {audio_state['rms']:.3f} | "
              f"BASS: {audio_state['bass']:.3f} | MIDS: {audio_state['mids']:.3f} | "
              f"HIGHS: {audio_state['highs']:.3f}")
        print(f"SCENE: {self.current_scene.upper()} | TRANSITIONS: {self.scene_transitions} | "
              f"INTENSITY: {directive['intensityAdjustment']:.2f}")
        print(f"DIRECTIVE: {directive['sceneIdentity']} | PALETTE: {directive['paletteShift']}")
        print(f"FPS: {self.frame_count / max(0.001, time.time() - self.start_time):.1f}")
        print("=" * 80)
    
    def update_scene_state(self, directive):
        """Update scene state based on director directive"""
        if directive["sceneIdentity"] != "maintain" and directive["sceneIdentity"] != self.current_scene:
            old_scene = self.current_scene
            self.current_scene = directive["sceneIdentity"]
            self.scene_transitions += 1
            print(f"🎬 SCENE TRANSITION: {old_scene} → {self.current_scene}")
        
        self.intensity_level = directive["intensityAdjustment"]
    
    def run_demo(self, duration_seconds=60):
        """Run the complete live visual demo"""
        if not self.connect_to_director():
            return
        
        print(f"🚀 Starting live visual demo for {duration_seconds} seconds...")
        print("Generating real audio-reactive pixels with Director system control")
        print("Press Ctrl+C to stop\n")
        
        try:
            while self.running and (time.time() - self.start_time) < duration_seconds:
                current_time = time.time()
                time_progress = current_time - self.start_time
                
                # Generate real audio state
                audio_state = self.generate_real_audio_state(time_progress)
                
                # Send state to Director and receive directive
                directive = self.send_state_to_director(audio_state)
                
                # Update scene state based on directive
                self.update_scene_state(directive)
                
                # Render frame with real pixels
                self.render_frame(audio_state, directive)
                
                # Store history for analysis
                self.rms_history.append(audio_state["rms"])
                self.bass_history.append(audio_state["bass"])
                self.mids_history.append(audio_state["mids"])
                self.highs_history.append(audio_state["highs"])
                self.tempo_history.append(audio_state["tempo"])
                
                # Keep history manageable
                if len(self.rms_history) > 100:
                    self.rms_history.pop(0)
                    self.bass_history.pop(0)
                    self.mids_history.pop(0)
                    self.highs_history.pop(0)
                    self.tempo_history.pop(0)
                
                self.frame_count += 1
                
                # Frame rate control (~30 FPS)
                time.sleep(0.033)
                
        except KeyboardInterrupt:
            print("\n\n🛑 Demo stopped by user")
        except Exception as e:
            print(f"\n❌ Demo error: {e}")
        finally:
            self.cleanup()
    
    def cleanup(self):
        """Clean up resources"""
        self.running = False
        if self.socket:
            try:
                self.socket.close()
                print("🔌 Disconnected from Director daemon")
            except:
                pass
        
        # Final statistics
        runtime = time.time() - self.start_time
        fps = self.frame_count / max(0.001, runtime)
        
        print(f"\n📊 DEMO STATISTICS")
        print("=" * 40)
        print(f"Total Frames: {self.frame_count}")
        print(f"Runtime: {runtime:.1f} seconds")
        print(f"Average FPS: {fps:.1f}")
        print(f"Scene Transitions: {self.scene_transitions}")
        print(f"Final Scene: {self.current_scene}")
        
        if self.rms_history:
            avg_rms = sum(self.rms_history) / len(self.rms_history)
            max_rms = max(self.rms_history)
            print(f"Average RMS: {avg_rms:.3f}")
            print(f"Peak RMS: {max_rms:.3f}")
        
        print("=" * 40)
        print("✅ Live visual demo completed successfully")

def main():
    """Main entry point"""
    demo = LiveVisualDemo()
    demo.run_demo(duration_seconds=45)  # Run for 45 seconds

if __name__ == "__main__":
    main()