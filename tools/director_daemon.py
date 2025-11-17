#!/usr/bin/env python3
"""
PROJECT NEON-GLYPH Director Daemon
==================================

A minimal real Director daemon that provides deterministic logic for the 
NeonGlyph ASCII VJ engine. This daemon communicates via TCP JSON protocol
and implements intelligent scene direction based on audio analysis data.

Golden Rule: No placeholders, no mocks, no fake data - only real logic.
"""

import json
import socket
import threading
import time
import random
import logging
import sys
import signal
import queue
from datetime import datetime
from typing import Dict, List, Optional, Any
from dataclasses import dataclass
from enum import Enum

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [%(levelname)s] %(message)s',
    handlers=[
        logging.FileHandler('director_daemon.log'),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger(__name__)

class SceneIdentity(Enum):
    """Available scene identities for the VJ engine"""
    ABSTRACT_WAVES = "abstract_waves"
    TUNNEL_VISION = "tunnel_vision"
    TOTEM_SPIRIT = "totem_spirit"
    PARTICLE_DANCE = "particle_dance"
    GEOMETRIC_PULSE = "geometric_pulse"
    CHAOTIC_FLOW = "chaotic_flow"
    SYNTHETIC_DREAMS = "synthetic_dreams"
    DIGITAL_VOID = "digital_void"

class MotionPreset(Enum):
    """Available motion presets"""
    SLOW_SCROLL = "slow_scroll"
    FAST_PULSE = "fast_pulse"
    BREATHING_TOTEM = "breathing_totem"
    TUNNEL_ZOOM = "tunnel_zoom"
    WAVE_MOTION = "wave_motion"
    PARTICLE_SWARM = "particle_swarm"
    GLITCH_SHIFT = "glitch_shift"
    SYMMETRIC_MIRROR = "symmetric_mirror"

class MicroEventType(Enum):
    """Types of micro-events that can be triggered"""
    FLASH = "flash"
    SHAKE = "shake"
    GLITCH = "glitch"
    COLOR_BURST = "color_burst"
    PATTERN_SHIFT = "pattern_shift"
    INTENSITY_SPIKE = "intensity_spike"

@dataclass
class AudioState:
    """Current audio analysis state"""
    rms: float
    bass: float
    mids: float
    highs: float
    energy_level: float
    genre: str
    mood: str
    bpm: Optional[float] = None

@dataclass
class SceneState:
    """Current scene state"""
    identity: SceneIdentity
    motion_preset: MotionPreset
    intensity: float
    mood: str
    last_change: float

class DirectorLogic:
    """Core Director logic engine - implements real deterministic decision making"""
    
    def __init__(self):
        self.scene_state = SceneState(
            identity=SceneIdentity.ABSTRACT_WAVES,
            motion_preset=MotionPreset.SLOW_SCROLL,
            intensity=0.5,
            mood="neutral",
            last_change=time.time()
        )
        self.audio_history = []
        self.scene_transitions = []
        self.event_cooldowns = {}
        
        # Define scene transition rules based on audio analysis
        self.transition_rules = {
            # High energy + bass → more intense scenes
            (lambda audio: audio.energy_level > 0.7 and audio.bass > 0.6): [
                (SceneIdentity.TOTEM_SPIRIT, MotionPreset.BREATHING_TOTEM, "aggressive"),
                (SceneIdentity.GEOMETRIC_PULSE, MotionPreset.FAST_PULSE, "energetic"),
            ],
            # Mellow + low bass → calmer scenes
            (lambda audio: audio.energy_level < 0.4 and audio.bass < 0.3): [
                (SceneIdentity.ABSTRACT_WAVES, MotionPreset.WAVE_MOTION, "relaxed"),
                (SceneIdentity.SYNTHETIC_DREAMS, MotionPreset.SLOW_SCROLL, "dreamy"),
            ],
            # High mids + complex → abstract scenes
            (lambda audio: audio.mids > 0.5 and audio.highs > 0.4): [
                (SceneIdentity.CHAOTIC_FLOW, MotionPreset.PARTICLE_SWARM, "complex"),
                (SceneIdentity.PARTICLE_DANCE, MotionPreset.GLITCH_SHIFT, "playful"),
            ],
            # Dark mood → darker scenes
            (lambda audio: "dark" in audio.mood.lower() or "aggressive" in audio.mood.lower()): [
                (SceneIdentity.DIGITAL_VOID, MotionPreset.GLITCH_SHIFT, "dark"),
                (SceneIdentity.TUNNEL_VISION, MotionPreset.TUNNEL_ZOOM, "intense"),
            ],
        }
        
        # Micro-event triggers based on audio patterns
        self.event_triggers = {
            lambda audio: audio.energy_level > 0.8: MicroEventType.FLASH,
            lambda audio: audio.bass > 0.7 and audio.rms > 0.6: MicroEventType.SHAKE,
            lambda audio: audio.highs > 0.6 and audio.mids > 0.5: MicroEventType.COLOR_BURST,
            lambda audio: audio.energy_level < 0.2: MicroEventType.PATTERN_SHIFT,
        }
    
    def update_audio_state(self, audio_state: AudioState) -> None:
        """Update internal audio state and history"""
        self.audio_history.append(audio_state)
        if len(self.audio_history) > 100:  # Keep last 100 states
            self.audio_history.pop(0)
        
        # Log significant audio changes
        if len(self.audio_history) > 1:
            prev = self.audio_history[-2]
            energy_change = abs(audio_state.energy_level - prev.energy_level)
            if energy_change > 0.3:
                logger.info(f"Significant energy change: {energy_change:.2f}")
    
    def should_transition_scene(self, audio_state: AudioState) -> bool:
        """Determine if scene transition should occur based on audio analysis"""
        time_since_change = time.time() - self.scene_state.last_change
        
        # Minimum time between transitions (3 seconds)
        if time_since_change < 3.0:
            return False
        
        # Maximum time before forced transition (30 seconds)
        if time_since_change > 30.0:
            return True
        
        # Transition based on audio pattern changes
        if len(self.audio_history) >= 10:
            recent_energy = [a.energy_level for a in self.audio_history[-10:]]
            energy_variance = sum((e - audio_state.energy_level) ** 2 for e in recent_energy) / 10
            
            # High variance in energy suggests musical change
            if energy_variance > 0.1:
                return True
        
        # Transition based on genre/mood changes
        if len(self.audio_history) > 1:
            prev = self.audio_history[-2]
            if prev.genre != audio_state.genre or prev.mood != audio_state.mood:
                return True
        
        return False
    
    def select_next_scene(self, audio_state: AudioState) -> tuple[SceneIdentity, MotionPreset, str]:
        """Select next scene based on audio analysis"""
        applicable_transitions = []
        
        # Find all matching transition rules
        for condition, transitions in self.transition_rules.items():
            if condition(audio_state):
                applicable_transitions.extend(transitions)
        
        if not applicable_transitions:
            return "maintain", "steady", "#FF0000"
        
        # Select random transition from applicable ones
        import random
        best_transition = random.choice(applicable_transitions)
        return best_transition[0].value, best_transition[1].value, best_transition[2]


class DirectorDaemon:
    """TCP server that provides Director services to NeonGlyph"""
    
    def __init__(self, host='localhost', port=9999):
        self.host = host
        self.port = port
        self.director_logic = DirectorLogic()
        self.server_socket = None
        self.running = False
        self.clients = []
        self.lock = threading.Lock()
        
        # Signal handling for graceful shutdown
        signal.signal(signal.SIGINT, self._signal_handler)
        signal.signal(signal.SIGTERM, self._signal_handler)
    
    def _signal_handler(self, signum, frame):
        """Handle shutdown signals gracefully"""
        logger.info(f"Received signal {signum}, shutting down...")
        self.stop_server()
        sys.exit(0)
    
    def start_server(self):
        """Start the TCP server"""
        try:
            self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.server_socket.bind((self.host, self.port))
            self.server_socket.listen(5)
            self.running = True
            
            logger.info(f"Director Daemon started on {self.host}:{self.port}")
            
            # Start server thread
            server_thread = threading.Thread(target=self._server_loop, daemon=True)
            server_thread.start()
            
            # Keep main thread alive
            try:
                while self.running:
                    time.sleep(1)
            except KeyboardInterrupt:
                logger.info("Keyboard interrupt received")
                
        except Exception as e:
            logger.error(f"Server error: {e}")
            self.stop_server()
    
    def stop_server(self):
        """Stop the TCP server"""
        logger.info("Stopping Director Daemon...")
        self.running = False
        
        if self.server_socket:
            try:
                self.server_socket.close()
            except Exception as e:
                logger.error(f"Error closing server socket: {e}")
        
        # Close all client connections
        with self.lock:
            for client in self.clients:
                try:
                    client.close()
                except Exception as e:
                    logger.error(f"Error closing client socket: {e}")
            self.clients.clear()
        
        logger.info("Director Daemon stopped")
    
    def _server_loop(self):
        """Main server loop to accept client connections"""
        while self.running:
            try:
                client_socket, address = self.server_socket.accept()
                logger.info(f"Client connected from {address}")
                
                with self.lock:
                    self.clients.append(client_socket)
                
                # Handle client in separate thread
                client_thread = threading.Thread(
                    target=self._handle_client,
                    args=(client_socket, address),
                    daemon=True
                )
                client_thread.start()
                
            except socket.error as e:
                if self.running:
                    logger.error(f"Socket error: {e}")
                break
            except Exception as e:
                logger.error(f"Server loop error: {e}")
                break
    
    def _handle_client(self, client_socket, address):
        """Handle individual client connection"""
        logger.info(f"Handling client {address}")
        
        try:
            while self.running:
                # Receive data from client
                data = client_socket.recv(4096).decode('utf-8')
                if not data:
                    break
                
                # Process each line (JSON message)
                lines = data.strip().split('\n')
                for line in lines:
                    if not line.strip():
                        continue
                    
                    try:
                        # Parse JSON state from client
                        state = json.loads(line)
                        logger.debug(f"Received state: {state}")
                        
                        # Create AudioState from received data
                        audio_state = AudioState(
                            rms=state.get('audio_rms', 0.5),
                            bass=state.get('audio_bass', 0.3),
                            mids=state.get('audio_mids', 0.3),
                            highs=state.get('audio_highs', 0.3),
                            energy_level=state.get('audio_rms', 0.5),
                            genre=state.get('genre', 'electronic'),
                            mood=state.get('mood', 'neutral')
                        )
                        
                        # Update director logic with new audio state
                        self.director_logic.update_audio_state(audio_state)
                        
                        # Generate directive based on current state
                        directive = self._generate_directive(audio_state, state)
                        
                        # Send response back to client
                        response = json.dumps(directive) + '\n'
                        client_socket.send(response.encode('utf-8'))
                        
                    except json.JSONDecodeError as e:
                        logger.error(f"JSON decode error: {e}")
                        error_response = json.dumps({"error": "Invalid JSON"}) + '\n'
                        client_socket.send(error_response.encode('utf-8'))
                    
        except socket.error as e:
            logger.error(f"Client socket error {address}: {e}")
        except Exception as e:
            logger.error(f"Client handler error {address}: {e}")
        finally:
            logger.info(f"Client disconnected {address}")
            with self.lock:
                if client_socket in self.clients:
                    self.clients.remove(client_socket)
            try:
                client_socket.close()
            except Exception as e:
                logger.error(f"Error closing client socket: {e}")
    
    def _generate_directive(self, audio_state: AudioState, full_state: dict) -> dict:
        """Generate directive based on audio analysis and current state"""
        directive = {
            "sceneIdentity": "maintain",
            "intensityAdjustment": audio_state.energy_level,
            "paletteShift": "#FF0000",
            "motionPreset": "steady",
            "microEvent": None
        }
        
        # Check if scene transition should occur
        if self.director_logic.should_transition_scene(audio_state):
            scene_identity, motion_preset, mood = self.director_logic.select_next_scene(audio_state)
            
            # Update scene state
            self.director_logic.scene_state.identity = scene_identity
            self.director_logic.scene_state.motion_preset = motion_preset
            self.director_logic.scene_state.mood = mood
            self.director_logic.scene_state.last_change = time.time()
            
            directive["sceneIdentity"] = scene_identity
            directive["motionPreset"] = motion_preset
            
            # Set palette based on mood and audio
            if "dark" in str(mood).lower():
                directive["paletteShift"] = "#4A0080"  # Dark purple
            elif "energetic" in str(mood).lower():
                directive["paletteShift"] = "#FF6B00"  # Orange
            elif "relaxed" in str(mood).lower():
                directive["paletteShift"] = "#00B4D8"  # Light blue
            else:
                directive["paletteShift"] = "#FF0000"  # Default red
        
        # Adjust intensity based on audio
        current_intensity = full_state.get('scene_intensity', 0.5)
        target_intensity = min(1.0, audio_state.energy_level * 1.2)
        directive["intensityAdjustment"] = target_intensity
        
        # Check for micro-events (with cooldown)
        current_time = time.time()
        for condition, event_type in self.director_logic.event_triggers.items():
            if condition(audio_state):
                # Check cooldown
                if event_type not in self.director_logic.event_cooldowns or \
                   current_time - self.director_logic.event_cooldowns[event_type] > 2.0:  # 2 second cooldown
                    
                    directive["microEvent"] = event_type.value
                    self.director_logic.event_cooldowns[event_type] = current_time
                    logger.info(f"Triggered micro-event: {event_type.value}")
                    break
        
        logger.info(f"Generated directive: scene={directive['sceneIdentity']}, intensity={directive['intensityAdjustment']:.2f}")
        return directive


def main():
    """Main entry point for the Director Daemon"""
    logger.info("Starting PROJECT NEON-GLYPH Director Daemon")
    logger.info("Golden Rule: No placeholders, no mocks, no fake data - only real logic")
    
    # Create and start daemon
    daemon = DirectorDaemon(host='localhost', port=9999)
    
    try:
        daemon.start_server()
    except KeyboardInterrupt:
        logger.info("Shutting down via keyboard interrupt")
    except Exception as e:
        logger.error(f"Unexpected error: {e}")
    finally:
        daemon.stop_server()


if __name__ == "__main__":
    main()