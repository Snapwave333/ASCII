#!/usr/bin/env python3
"""
Console-mode demonstration of NeonGlyph with LLM integration
Simulates the application running in console-only mode with audio input
"""

import time
import random
import math

def simulate_audio_analysis():
    """Simulate audio analysis data"""
    return {
        'bpm': 128.0 + random.uniform(-5, 5),
        'key': random.choice(['C minor', 'D major', 'A minor', 'F major', 'G minor']),
        'energy': random.uniform(0.3, 0.9),
        'song_section': random.choice(['intro', 'verse', 'chorus', 'bridge', 'outro']),
        'beat_phase': random.uniform(0, 1),
        'dynamics': random.uniform(0.4, 0.8),
        'rms_level_db': random.uniform(-25, -10)
    }

def generate_ascii_frame(width, height, music_data, frame_count):
    """Generate a simple ASCII art frame based on music data"""
    frame = []
    
    # Header
    frame.append("=" * width)
    frame.append("NEON-GLYPH CONSOLE MODE - AI-Driven ASCII Visualizer".center(width))
    frame.append("=" * width)
    
    # Audio info
    frame.append(f"BPM: {music_data['bpm']:.1f} | Key: {music_data['key']} | Section: {music_data['song_section']}")
    frame.append(f"Energy: {music_data['energy']:.2f} | Dynamics: {music_data['dynamics']:.2f}")
    frame.append("")
    
    # AI Director simulation
    scene_id = f"SCN-{frame_count:03d}"
    narrative_beat = random.choice(['Setup', 'Confrontation', 'Resolution', 'Climax'])
    palette = random.choice(['cyberpunk', 'vaporwave', 'matrix', 'noir'])
    
    frame.append(f"Scene: {scene_id} | Narrative: {narrative_beat}")
    frame.append(f"Palette: {palette} | ASCII Grid: {width}x{height}")
    frame.append("")
    
    # ASCII visualization based on energy
    energy_chars = " .-:=+*#%@"
    energy_index = int(music_data['energy'] * (len(energy_chars) - 1))
    
    # Create a simple pattern
    pattern_width = min(60, width - 4)
    pattern_height = min(15, height - 12)
    
    for y in range(pattern_height):
        line = "  "
        for x in range(pattern_width):
            # Create wave pattern based on beat phase and position
            wave = math.sin((x / pattern_width * 2 * math.pi) + (music_data['beat_phase'] * 2 * math.pi))
            intensity = (wave + 1) / 2 * music_data['energy']
            char_index = int(intensity * (len(energy_chars) - 1))
            line += energy_chars[char_index]
        frame.append(line)
    
    # Footer
    frame.append("")
    frame.append("LLM Prompt Integration: ACTIVE")
    frame.append("High-res ASCII Animation Director persona applied to all AI requests")
    frame.append("Press Ctrl+C to exit")
    
    return frame

def main():
    print("=== NeonGlyph Console Mode Demo ===")
    print("Simulating AI-driven ASCII visualization with LLM integration")
    print("This demonstrates how the application would run in console-only mode")
    print("with the high-resolution ASCII animation director persona active.\n")
    
    width = 80
    height = 25
    frame_count = 0
    
    try:
        while True:
            # Clear screen
            print("\033[2J\033[H", end="")
            
            # Simulate audio analysis
            music_data = simulate_audio_analysis()
            
            # Generate ASCII frame
            frame = generate_ascii_frame(width, height, music_data, frame_count)
            
            # Display frame
            for line in frame:
                print(line)
            
            # Show LLM integration status
            print(f"\nFrame: {frame_count} | LLM Role Prompt: ACTIVE")
            print("AI Director generating high-res ASCII animation directives...")
            
            frame_count += 1
            time.sleep(0.2)  # Simulate 5 FPS
            
    except KeyboardInterrupt:
        print("\n\nDemo terminated by user.")
        print("In the real application, this would be running with live audio input")
        print("and the LLM would be generating actual ASCII animation directives.")

if __name__ == "__main__":
    main()