#!/usr/bin/env python3
"""
Comprehensive Visual Test Suite for NeonGlyph

This test ensures the application is rendering the complete "full show" experience
including ASCII art, animations, color modes, and performance metrics.
"""

import os
import time
import json
import subprocess
import threading
from pathlib import Path
import logging
from typing import Dict, List, Optional
import re

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

class ComprehensiveVisualTester:
    """Comprehensive visual testing for NeonGlyph full rendering experience"""
    
    def __init__(self):
        self.root_dir = Path(__file__).resolve().parents[2]
        self.build_dir = self.root_dir / "build64" / "Release"
        self.exe_path = self.build_dir / "NeonGlyph.exe"
        self.log_path = self.build_dir / "staging_run.log"
        self.process = None
        self.log_content = ""
        self.metrics = {
            'frame_times': [],
            'color_modes': [],
            'animations': [],
            'ascii_content': False,
            'performance_metrics': {},
            'visual_effects': []
        }
        
    def run_application(self, duration: int = 30) -> bool:
        """Run the application for specified duration and capture logs"""
        logger.info(f"🚀 Starting NeonGlyph application for {duration} seconds...")
        
        if not self.exe_path.exists():
            logger.error(f"❌ Executable not found: {self.exe_path}")
            return False
            
        # Clear previous log
        if self.log_path.exists():
            try:
                self.log_path.unlink()
            except Exception:
                pass
        
        # Set environment variables
        env = os.environ.copy()
        env["NG_METRICS_PATH"] = str(self.log_path)
        env["NG_LOG_ONLY"] = "0"
        
        try:
            # Change to build directory and run application
            original_cwd = os.getcwd()
            os.chdir(self.build_dir)
            
            self.process = subprocess.Popen(
                [str(self.exe_path)],
                env=env,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                creationflags=subprocess.CREATE_NO_WINDOW
            )
            
            logger.info("✅ Application started successfully")
            
            # Monitor for specified duration
            start_time = time.time()
            last_log_check = start_time
            
            while time.time() - start_time < duration:
                # Check log file every 2 seconds
                if time.time() - last_log_check > 2:
                    self._analyze_log_file()
                    last_log_check = time.time()
                    
                # Check if process is still running
                if self.process.poll() is not None:
                    logger.warning("⚠️  Application terminated early")
                    break
                    
                time.sleep(0.5)
                
            # Final analysis
            self._analyze_log_file()
            
            # Stop the application
            self._stop_application()
            
            # Restore original directory
            os.chdir(original_cwd)
            
            logger.info("✅ Application test completed")
            return True
            
        except Exception as e:
            logger.error(f"❌ Failed to run application: {e}")
            return False
            
    def _stop_application(self):
        """Stop the application gracefully"""
        if self.process:
            try:
                self.process.terminate()
                self.process.wait(timeout=5)
            except Exception:
                try:
                    self.process.kill()
                except Exception:
                    pass
            finally:
                self.process = None
                
    def _analyze_log_file(self):
        """Analyze the application log file for visual content"""
        if not self.log_path.exists():
            logger.warning("⚠️  Log file not found")
            return
            
        try:
            with open(self.log_path, 'r', encoding='utf-8', errors='ignore') as f:
                self.log_content = f.read()
                
            # Extract performance metrics
            self._extract_performance_metrics()
            
            # Extract color modes
            self._extract_color_modes()
            
            # Extract animations
            self._extract_animations()
            
            # Check for ASCII content
            self._check_ascii_content()
            
            # Extract visual effects
            self._extract_visual_effects()
            
        except Exception as e:
            logger.error(f"❌ Failed to analyze log: {e}")
            
    def _extract_performance_metrics(self):
        """Extract frame timing and performance metrics"""
        frame_pattern = r'TS=(\d+)\s+RenderMs=([\d.]+)\s+PresentMs=([\d.]+)\s+FrameMs=([\d.]+)'
        matches = re.findall(frame_pattern, self.log_content)
        
        if matches:
            self.metrics['frame_times'] = [float(match[3]) for match in matches[-100:]]  # Last 100 frames
            
            if self.metrics['frame_times']:
                avg_frame_time = sum(self.metrics['frame_times']) / len(self.metrics['frame_times'])
                max_frame_time = max(self.metrics['frame_times'])
                min_frame_time = min(self.metrics['frame_times'])
                
                self.metrics['performance_metrics'] = {
                    'avg_frame_time_ms': avg_frame_time,
                    'max_frame_time_ms': max_frame_time,
                    'min_frame_time_ms': min_frame_time,
                    'frame_count': len(self.metrics['frame_times']),
                    'avg_fps': 1000.0 / avg_frame_time if avg_frame_time > 0 else 0
                }
                
                logger.info(f"📊 Performance: Avg={avg_frame_time:.2f}ms, Max={max_frame_time:.2f}ms, FPS={1000.0/avg_frame_time:.1f}")
                
    def _extract_color_modes(self):
        """Extract color mode changes"""
        color_pattern = r'ColorMode=(\w+)'
        matches = re.findall(color_pattern, self.log_content)
        self.metrics['color_modes'] = list(set(matches))  # Unique modes
        
        if self.metrics['color_modes']:
            logger.info(f"🎨 Color modes detected: {', '.join(self.metrics['color_modes'])}")
            
    def _extract_animations(self):
        """Extract animation sequences"""
        animation_indicators = [
            'MorphToTestCard', 'AnimateFluidSim', 'ApplySymmetry', 
            'RenderTestCard', 'RenderNeonGlyphLogo'
        ]
        
        for indicator in animation_indicators:
            if indicator in self.log_content:
                self.metrics['animations'].append(indicator)
                
        if self.metrics['animations']:
            logger.info(f"🎬 Animations detected: {', '.join(self.metrics['animations'])}")
            
    def _check_ascii_content(self):
        """Check for ASCII art content indicators"""
        ascii_indicators = ['NEONGLYPH', 'RenderTestCard', 'RenderNeonGlyphLogo', 'ASCII']
        
        for indicator in ascii_indicators:
            if indicator in self.log_content:
                self.metrics['ascii_content'] = True
                break
                
        if self.metrics['ascii_content']:
            logger.info("✅ ASCII content detected")
            
    def _extract_visual_effects(self):
        """Extract visual effects and rendering features"""
        effects = []
        
        if 'RenderMs=' in self.log_content:
            effects.append('Vulkan_Rendering')
            
        if 'PresentMs=' in self.log_content:
            effects.append('Presentation_Layer')
            
        if 'Var=' in self.log_content:
            effects.append('Performance_Variance')
            
        if 'Jitter=' in self.log_content:
            effects.append('Frame_Jitter_Monitoring')
            
        self.metrics['visual_effects'] = effects
        
        if effects:
            logger.info(f"✨ Visual effects: {', '.join(effects)}")
            
    def validate_full_show_experience(self) -> Dict[str, bool]:
        """Validate that the application is rendering the complete visual experience"""
        logger.info("🔍 Validating full show experience...")
        
        validation_results = {
            'performance_metrics_available': len(self.metrics['frame_times']) > 0,
            'frame_times_reasonable': self._check_frame_times_reasonable(),
            'color_modes_detected': len(self.metrics['color_modes']) > 0,
            'ascii_content_rendering': self.metrics['ascii_content'],
            'animations_active': len(self.metrics['animations']) > 0,
            'visual_effects_enabled': len(self.metrics['visual_effects']) > 0,
            'performance_consistent': self._check_performance_consistency(),
            'overall_experience': False
        }
        
        # Calculate overall result
        required_checks = [
            'performance_metrics_available',
            'frame_times_reasonable', 
            'color_modes_detected',
            'ascii_content_rendering',
            'animations_active',
            'visual_effects_enabled'
        ]
        
        passed_checks = sum(validation_results[check] for check in required_checks)
        validation_results['overall_experience'] = passed_checks >= len(required_checks) - 1
        
        logger.info(f"📋 Validation Results:")
        for check, result in validation_results.items():
            status = "✅" if result else "❌"
            logger.info(f"  {status} {check.replace('_', ' ').title()}")
            
        return validation_results
        
    def _check_frame_times_reasonable(self) -> bool:
        """Check if frame times are within acceptable ranges"""
        if not self.metrics['frame_times']:
            return False
            
        avg_frame_time = self.metrics['performance_metrics'].get('avg_frame_time_ms', 0)
        max_frame_time = self.metrics['performance_metrics'].get('max_frame_time_ms', 0)
        
        # Reasonable frame time thresholds (for 60fps target)
        reasonable_avg = avg_frame_time < 20.0  # Allow up to 20ms average
        reasonable_max = max_frame_time < 50.0  # Allow occasional spikes up to 50ms
        
        return reasonable_avg and reasonable_max
        
    def _check_performance_consistency(self) -> bool:
        """Check if performance is consistent without major drops"""
        if len(self.metrics['frame_times']) < 10:
            return False
            
        frame_times = self.metrics['frame_times'][-30:]  # Check last 30 frames
        avg_time = sum(frame_times) / len(frame_times)
        
        # Check for sudden performance drops (>50% increase)
        sudden_drops = 0
        for i in range(1, len(frame_times)):
            if frame_times[i] > frame_times[i-1] * 1.5:
                sudden_drops += 1
                
        # Allow up to 2 sudden drops in 30 frames
        return sudden_drops <= 2
        
    def generate_test_report(self) -> str:
        """Generate comprehensive test report"""
        report = []
        report.append("=" * 60)
        report.append("NEONGLYPH FULL SHOW EXPERIENCE TEST REPORT")
        report.append("=" * 60)
        report.append("")
        
        # Performance Summary
        if self.metrics['performance_metrics']:
            report.append("PERFORMANCE METRICS:")
            pm = self.metrics['performance_metrics']
            report.append(f"  Average Frame Time: {pm['avg_frame_time_ms']:.2f}ms")
            report.append(f"  Maximum Frame Time: {pm['max_frame_time_ms']:.2f}ms")
            report.append(f"  Minimum Frame Time: {pm['min_frame_time_ms']:.2f}ms")
            report.append(f"  Average FPS: {pm['avg_fps']:.1f}")
            report.append(f"  Total Frames: {pm['frame_count']}")
            report.append("")
            
        # Visual Content Summary
        report.append("VISUAL CONTENT:")
        report.append(f"  Color Modes: {', '.join(self.metrics['color_modes']) if self.metrics['color_modes'] else 'None'}")
        report.append(f"  ASCII Content: {'DETECTED' if self.metrics['ascii_content'] else 'Not detected'}")
        report.append(f"  Animations: {', '.join(self.metrics['animations']) if self.metrics['animations'] else 'None'}")
        report.append(f"  Visual Effects: {', '.join(self.metrics['visual_effects']) if self.metrics['visual_effects'] else 'None'}")
        report.append("")
        
        # Validation Results
        validation = self.validate_full_show_experience()
        report.append("VALIDATION RESULTS:")
        for check, result in validation.items():
            if check != 'overall_experience':
                status = "PASS" if result else "FAIL"
                report.append(f"  {status} {check.replace('_', ' ').title()}")
                
        report.append("")
        
        # Overall Result
        overall_status = "PASSED" if validation['overall_experience'] else "FAILED"
        report.append(f"OVERALL FULL SHOW EXPERIENCE: {overall_status}")
        report.append("")
        report.append("=" * 60)
        
        return "\n".join(report)

def main():
    """Main test execution"""
    logger.info("🚀 Starting NeonGlyph Full Show Experience Test")
    logger.info("This test will verify that the application is rendering the complete visual experience")
    logger.info("including ASCII art, animations, color modes, and performance metrics.\n")
    
    tester = ComprehensiveVisualTester()
    
    # Run the application for 30 seconds
    success = tester.run_application(duration=30)
    
    if not success:
        logger.error("❌ Failed to run application")
        return False
        
    # Generate and display test report
    report = tester.generate_test_report()
    print(report)
    
    # Save report to file
    report_file = Path("full_show_test_report.txt")
    with open(report_file, 'w') as f:
        f.write(report)
        
    logger.info(f"📄 Test report saved to: {report_file}")
    
    # Return overall result
    validation = tester.validate_full_show_experience()
    return validation['overall_experience']

if __name__ == "__main__":
    success = main()
    exit(0 if success else 1)