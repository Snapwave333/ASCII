#!/usr/bin/env python3
"""
NeonGlyph Pure Visual Mode Demo
Clean ASCII visuals with NO HUD elements for viewers
AI processes all data internally while viewers see only pure visuals
"""

import os
import sys
import time
import json
import random
import math
import threading
from datetime import datetime

class PureVisualDemo:
    def __init__(self):
        self.config = self.load_config()
        self.running = True
        self.audio_reactive = True
        self.frame_count = 0
        self.last_time = time.time()
        self.fps = 0
        
        # ASCII patterns for cyberpunk @ symbol theme
        self.at_patterns = [
            " @ ",
            "@ @",
            " @@",
            "@  ",
            " @@",
            "@ @",
            " @ "
        ]
        
        # Color codes for terminal (hidden from viewers)
        self.colors = {
            'cyan': '\033[96m',
            'pink': '\033[95m',
            'green': '\033[92m',
            'yellow': '\033[93m',
            'red': '\033[91m',
            'reset': '\033[0m'
        }
        
        # Audio simulation data (internal AI processing)
        self.audio_data = {
            'bass': 0.0,
            'mid': 0.0,
            'treble': 0.0,
            'volume': 0.0,
            'beat_detected': False,
            'beat_intensity': 0.0
        }
        
        # Performance metrics (internal AI processing)
        self.performance_data = {
            'fps': 0,
            'frame_time': 0,
            'memory_usage': 0,
            'cpu_usage': 0
        }
        
        # AI state (internal processing)
        self.ai_state = {
            'mode': 'running',
            'visual_style': 'cyberpunk',
            'reactivity_level': 'high',
            'complexity': 'medium'
        }

    def load_config(self):
        """Load pure visual mode configuration"""
        try:
            config_path = os.path.join('config', 'pure_visual_mode.json')
            with open(config_path, 'r') as f:
                return json.load(f)
        except FileNotFoundError:
            return {
                'pure_visual_mode': {'enabled': True},
                'ai_internal_display': {'enabled': True},
                'viewer_experience': {'clean_ascii_only': True}
            }

    def simulate_audio_data(self):
        """Simulate audio analysis data for AI processing (internal only)"""
        # Generate realistic audio data patterns
        t = time.time()
        
        # Bass frequencies (low, rhythmic)
        self.audio_data['bass'] = 0.3 + 0.4 * math.sin(t * 2.5) + 0.2 * random.random()
        
        # Mid frequencies (melodic content)
        self.audio_data['mid'] = 0.4 + 0.3 * math.sin(t * 4.2) + 0.15 * random.random()
        
        # Treble frequencies (high details)
        self.audio_data['treble'] = 0.2 + 0.3 * math.sin(t * 8.7) + 0.1 * random.random()
        
        # Overall volume
        self.audio_data['volume'] = (self.audio_data['bass'] + self.audio_data['mid'] + self.audio_data['treble']) / 3
        
        # Beat detection simulation
        beat_threshold = 0.7
        if self.audio_data['bass'] > beat_threshold and not self.audio_data['beat_detected']:
            self.audio_data['beat_detected'] = True
            self.audio_data['beat_intensity'] = min(1.0, self.audio_data['bass'])
        elif self.audio_data['bass'] < beat_threshold * 0.7:
            self.audio_data['beat_detected'] = False
            self.audio_data['beat_intensity'] *= 0.9  # Decay

    def update_performance_metrics(self):
        """Update performance metrics (internal AI processing)"""
        current_time = time.time()
        delta_time = current_time - self.last_time
        self.last_time = current_time
        
        if delta_time > 0:
            self.performance_data['fps'] = 1.0 / delta_time
            self.performance_data['frame_time'] = delta_time * 1000  # ms
        
        # Simulate memory and CPU usage
        self.performance_data['memory_usage'] = 45 + 15 * math.sin(current_time * 0.5)
        self.performance_data['cpu_usage'] = 25 + 20 * math.sin(current_time * 1.2)

    def generate_pure_ascii_visual(self):
        """Generate clean ASCII visuals with NO information overlays"""
        # Clear screen
        os.system('cls' if os.name == 'nt' else 'clear')
        
        # Get terminal size
        try:
            rows, cols = os.get_terminal_size()
        except:
            rows, cols = 24, 80
        
        # Simulate audio reactivity for visual generation
        self.simulate_audio_data()
        
        # Generate cyberpunk @ symbol patterns
        visual_output = []
        
        # Top border
        visual_output.append("═" * cols)
        
        # Main visual area - pure ASCII only
        center_y = rows // 2
        center_x = cols // 2
        
        for y in range(3, rows - 3):
            line = ""
            for x in range(cols):
                # Create cyberpunk @ symbol patterns with audio reactivity
                distance_from_center = math.sqrt((x - center_x)**2 + (y - center_y)**2)
                
                # Audio-reactive pattern generation
                bass_intensity = self.audio_data['bass']
                mid_intensity = self.audio_data['mid']
                treble_intensity = self.audio_data['treble']
                
                # Pattern selection based on audio
                pattern_index = int((distance_from_center / 10 + self.frame_count * 0.1) * bass_intensity) % len(self.at_patterns)
                
                # Create flowing @ symbol patterns
                if distance_from_center < 20 + bass_intensity * 10:
                    if (x + y + self.frame_count) % int(8 - treble_intensity * 4) == 0:
                        line += "@"
                    elif (x - y + self.frame_count) % int(6 - mid_intensity * 3) == 0:
                        line += "●"
                    else:
                        line += " "
                elif distance_from_center < 35 + mid_intensity * 15:
                    if random.random() < 0.1 + treble_intensity * 0.2:
                        line += "·"
                    else:
                        line += " "
                else:
                    if random.random() < 0.02 + bass_intensity * 0.05:
                        line += "◦"
                    else:
                        line += " "
            
            visual_output.append(line)
        
        # Bottom border
        visual_output.append("═" * cols)
        
        # Return pure visual output only - NO information overlays
        return "\n".join(visual_output)

    def process_ai_internal_data(self):
        """Process all AI data internally (NOT displayed to viewers)"""
        self.update_performance_metrics()
        
        # Log internal data to console/file (not visible to viewers)
        if self.config['ai_internal_display']['enabled']:
            internal_info = {
                'timestamp': datetime.now().isoformat(),
                'frame': self.frame_count,
                'fps': round(self.performance_data['fps'], 1),
                'audio': {
                    'bass': round(self.audio_data['bass'], 2),
                    'mid': round(self.audio_data['mid'], 2),
                    'treble': round(self.audio_data['treble'], 2),
                    'volume': round(self.audio_data['volume'], 2),
                    'beat': self.audio_data['beat_detected']
                },
                'ai_state': self.ai_state['mode'],
                'memory': round(self.performance_data['memory_usage'], 1),
                'cpu': round(self.performance_data['cpu_usage'], 1)
            }
            
            # Write to log file (internal processing only)
            try:
                with open('ai_internal_log.jsonl', 'a') as f:
                    f.write(json.dumps(internal_info) + '\n')
            except:
                pass  # Silent fail - viewers don't need to know

    def run(self):
        """Main demo loop - pure visuals for viewers, AI processing internally"""
        print("NeonGlyph Pure Visual Mode")
        print("=" * 40)
        print("Viewers see: Clean ASCII visuals only")
        print("AI processes: All data internally")
        print("Controls: F11 (fullscreen), ESC (exit fullscreen)")
        print("          Alt+P (start/pause), Alt+S (stop), Alt+B (break)")
        print("          Double-click (borderless toggle)")
        print("=" * 40)
        print("Starting pure visual experience...")
        time.sleep(2)
        
        try:
            while self.running:
                # Generate pure visual output (NO HUD elements)
                visual_output = self.generate_pure_ascii_visual()
                print(visual_output)
                
                # Process AI data internally (viewers don't see this)
                self.process_ai_internal_data()
                
                # Update frame counter
                self.frame_count += 1
                
                # Control frame rate
                time.sleep(0.033)  # ~30 FPS
                
        except KeyboardInterrupt:
            self.running = False
            print("\nPure visual demo stopped.")

if __name__ == "__main__":
    demo = PureVisualDemo()
    demo.run()