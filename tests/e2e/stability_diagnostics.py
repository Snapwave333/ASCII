import os
import time
import subprocess
import json
from pathlib import Path
import logging
from datetime import datetime

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

ROOT = Path(__file__).resolve().parents[2]
BIN = ROOT / "build64" / "Release" / "NeonGlyph.exe"
LOG = BIN.parent / "staging_run.log"
CRASH_LOG = ROOT / "crash_analysis.log"

class StabilityDiagnostics:
    def __init__(self):
        self.crashes_detected = 0
        self.successful_runs = 0
        self.crash_patterns = []
        
    def analyze_crash_patterns(self):
        """Analyze the log file for crash patterns"""
        logger.info("Analyzing crash patterns...")
        
        if not LOG.exists():
            logger.warning("No log file found")
            return
            
        try:
            log_content = LOG.read_text(encoding="utf-8", errors="ignore")
            lines = log_content.split('\n')
            
            # Look for incomplete startup sequences
            startup_sequences = []
            current_sequence = []
            
            for line in lines:
                if "Version 3.0.1 Startup" in line:
                    if current_sequence:
                        startup_sequences.append(current_sequence)
                    current_sequence = [line]
                elif current_sequence:
                    current_sequence.append(line)
            
            if current_sequence:
                startup_sequences.append(current_sequence)
            
            # Analyze each sequence for completeness
            incomplete_sequences = []
            for i, sequence in enumerate(startup_sequences):
                has_startup = any("Startup Window+GLFW initialized" in line for line in sequence)
                has_vulkan = any("CreateVulkanSurfaceMs" in line for line in sequence)
                has_rendering = any("RenderMs" in line or "FrameMs" in line for line in sequence)
                
                sequence_info = {
                    'sequence_id': i + 1,
                    'total_lines': len(sequence),
                    'has_startup': has_startup,
                    'has_vulkan': has_vulkan,
                    'has_rendering': has_rendering,
                    'complete': has_startup and has_vulkan and has_rendering,
                    'first_line': sequence[0] if sequence else "",
                    'last_line': sequence[-1] if sequence else ""
                }
                
                if not sequence_info['complete']:
                    incomplete_sequences.append(sequence_info)
                    self.crashes_detected += 1
                else:
                    self.successful_runs += 1
                    
            logger.info(f"Total startup sequences: {len(startup_sequences)}")
            logger.info(f"Successful runs: {self.successful_runs}")
            logger.info(f"Crashes detected: {self.crashes_detected}")
            
            if incomplete_sequences:
                logger.info("Incomplete sequences (potential crashes):")
                for seq in incomplete_sequences:
                    logger.info(f"  Sequence {seq['sequence_id']}: {seq['first_line']} -> {seq['last_line']}")
                    logger.info(f"    Startup: {seq['has_startup']}, Vulkan: {seq['has_vulkan']}, Rendering: {seq['has_rendering']}")
            
            return {
                'total_sequences': len(startup_sequences),
                'successful_runs': self.successful_runs,
                'crashes_detected': self.crashes_detected,
                'crash_rate': self.crashes_detected / len(startup_sequences) if startup_sequences else 0,
                'incomplete_sequences': incomplete_sequences
            }
            
        except Exception as e:
            logger.error(f"Failed to analyze crash patterns: {e}")
            return None
    
    def stress_test_application(self, duration=60, max_concurrent_runs=5):
        """Stress test the application for stability"""
        logger.info(f"Starting stress test for {duration} seconds...")
        
        start_time = time.time()
        test_results = []
        run_count = 0
        
        while time.time() - start_time < duration:
            run_count += 1
            logger.info(f"Test run {run_count}")
            
            # Clear previous log
            if LOG.exists():
                try:
                    LOG.unlink()
                except Exception:
                    pass
            
            # Start application
            process = None
            run_start = time.time()
            
            try:
                env = os.environ.copy()
                env["NG_LOG_ONLY"] = "0"
                env["NG_METRICS_PATH"] = str(LOG)
                
                process = subprocess.Popen(
                    str(BIN),
                    cwd=str(BIN.parent),
                    env=env,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE
                )
                
                # Let it run for a short time
                time.sleep(3)
                
                # Check if still running
                return_code = process.poll()
                if return_code is None:
                    # Still running, terminate gracefully
                    process.terminate()
                    try:
                        process.wait(timeout=5)
                        run_result = {
                            'run_id': run_count,
                            'duration': time.time() - run_start,
                            'status': 'success',
                            'return_code': 0,
                            'crashed': False
                        }
                    except subprocess.TimeoutExpired:
                        process.kill()
                        run_result = {
                            'run_id': run_count,
                            'duration': time.time() - run_start,
                            'status': 'timeout',
                            'return_code': -1,
                            'crashed': False
                        }
                else:
                    # Process exited
                    run_result = {
                        'run_id': run_count,
                        'duration': time.time() - run_start,
                        'status': 'crashed' if return_code != 0 else 'exited',
                        'return_code': return_code,
                        'crashed': return_code != 0
                    }
                    
                # Analyze the log for this run
                if LOG.exists():
                    log_content = LOG.read_text(encoding="utf-8", errors="ignore")
                    has_startup = "Startup Window+GLFW initialized" in log_content
                    has_vulkan = "CreateVulkanSurfaceMs" in log_content
                    has_rendering = "RenderMs" in log_content or "FrameMs" in log_content
                    
                    run_result['log_analysis'] = {
                        'has_startup': has_startup,
                        'has_vulkan': has_vulkan,
                        'has_rendering': has_rendering,
                        'progress': sum([has_startup, has_vulkan, has_rendering])
                    }
                else:
                    run_result['log_analysis'] = {
                        'has_startup': False,
                        'has_vulkan': False,
                        'has_rendering': False,
                        'progress': 0
                    }
                
                test_results.append(run_result)
                
            except Exception as e:
                logger.error(f"Run {run_count} failed with exception: {e}")
                if process:
                    try:
                        process.kill()
                    except:
                        pass
                
                test_results.append({
                    'run_id': run_count,
                    'duration': time.time() - run_start,
                    'status': 'exception',
                    'return_code': -1,
                    'crashed': True,
                    'error': str(e)
                })
            
            # Brief pause between runs
            time.sleep(1)
        
        # Analyze results
        crashed_runs = sum(1 for r in test_results if r['crashed'])
        successful_runs = len(test_results) - crashed_runs
        avg_progress = sum(r.get('log_analysis', {}).get('progress', 0) for r in test_results) / len(test_results) if test_results else 0
        
        logger.info(f"Stress test completed:")
        logger.info(f"  Total runs: {len(test_results)}")
        logger.info(f"  Successful: {successful_runs}")
        logger.info(f"  Crashed: {crashed_runs}")
        logger.info(f"  Crash rate: {crashed_runs/len(test_results)*100:.1f}%")
        logger.info(f"  Average progress: {avg_progress:.1f}/3")
        
        return {
            'total_runs': len(test_results),
            'successful_runs': successful_runs,
            'crashed_runs': crashed_runs,
            'crash_rate': crashed_runs / len(test_results) if test_results else 0,
            'average_progress': avg_progress,
            'detailed_results': test_results
        }

def main():
    """Run comprehensive stability diagnostics"""
    diagnostics = StabilityDiagnostics()
    
    print("🔍 NeonGlyph Stability Diagnostics")
    print("=" * 50)
    
    # Analyze existing crash patterns
    print("\n📊 Analyzing existing crash patterns...")
    crash_analysis = diagnostics.analyze_crash_patterns()
    
    if crash_analysis:
        print(f"Crash Rate: {crash_analysis['crash_rate']*100:.1f}%")
        print(f"Successful Runs: {crash_analysis['successful_runs']}")
        print(f"Crashes Detected: {crash_analysis['crashes_detected']}")
    
    # Run stress test
    print("\n⚡ Running stress test...")
    stress_results = diagnostics.stress_test_application(duration=30, max_concurrent_runs=3)
    
    if stress_results:
        print(f"Stress Test Crash Rate: {stress_results['crash_rate']*100:.1f}%")
        print(f"Average Initialization Progress: {stress_results['average_progress']:.1f}/3")
    
    # Generate report
    print("\n📋 Generating detailed report...")
    report = {
        'timestamp': datetime.now().isoformat(),
        'existing_crashes': crash_analysis,
        'stress_test_results': stress_results,
        'recommendations': []
    }
    
    # Add recommendations based on results
    if crash_analysis and crash_analysis['crash_rate'] > 0.5:
        report['recommendations'].append("High crash rate detected - investigate initialization sequence")
    
    if stress_results and stress_results['average_progress'] < 2.0:
        report['recommendations'].append("Low initialization progress - check Vulkan/graphics initialization")
    
    if stress_results and stress_results['crash_rate'] > 0.3:
        report['recommendations'].append("Stress test shows instability - consider memory/performance optimization")
    
    # Save report
    try:
        with open(CRASH_LOG, 'w') as f:
            json.dump(report, f, indent=2)
        print(f"Report saved to: {CRASH_LOG}")
    except Exception as e:
        print(f"Failed to save report: {e}")
    
    print("\n✅ Stability diagnostics completed!")
    
    return report

if __name__ == "__main__":
    main()