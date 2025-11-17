import os
import time
import subprocess
from pathlib import Path
import logging

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

ROOT = Path(__file__).resolve().parents[2]
BIN = ROOT / "build64" / "Release" / "NeonGlyph.exe"
LOG = BIN.parent / "staging_run.log"

def test_full_rendering_mode():
    """Test if the application enters full rendering mode with visual output"""
    logger.info("🎨 Testing Full Rendering Mode")
    logger.info("=" * 40)
    
    # Clear previous log
    if LOG.exists():
        LOG.unlink()
    
    try:
        env = os.environ.copy()
        env["NG_LOG_ONLY"] = "0"
        env["NG_METRICS_PATH"] = str(LOG)
        
        logger.info("Starting application with windowed mode...")
        process = subprocess.Popen(
            [str(BIN), "--no-headless-fallback"],
            cwd=str(BIN.parent),
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        
        logger.info(f"Process started with PID: {process.pid}")
        
        # Monitor for 15 seconds to see if rendering starts
        rendering_detected = False
        frame_tick_detected = False
        
        for i in range(15):
            time.sleep(1)
            
            # Check if process is still running
            return_code = process.poll()
            if return_code is not None:
                logger.info(f"Process exited after {i+1} seconds with code: {return_code}")
                break
            
            # Check log for rendering activity
            if LOG.exists():
                log_content = LOG.read_text(encoding="utf-8", errors="ignore")
                
                # Look for rendering indicators
                if "RenderLoop FrameTick=" in log_content:
                    frame_tick_detected = True
                    logger.info(f"✅ FrameTick detected after {i+1} seconds!")
                
                if "RenderMs=" in log_content or "FrameMs=" in log_content:
                    rendering_detected = True
                    logger.info(f"✅ Rendering metrics detected after {i+1} seconds!")
                
                # Look for visual content indicators
                if "NEONGLYPH" in log_content:
                    logger.info(f"✅ NEONGLYPH content detected after {i+1} seconds!")
                
                if "ColorMode=" in log_content:
                    logger.info(f"✅ Color mode detected after {i+1} seconds!")
                
                # Count total log entries
                log_entries = len([line for line in log_content.split('\n') if line.strip()])
                if log_entries > 10 and i > 5:
                    logger.info(f"📊 Log entries: {log_entries}")
        
        # Terminate process
        logger.info("Terminating application...")
        process.terminate()
        try:
            process.wait(timeout=5)
            logger.info("Application terminated gracefully")
        except subprocess.TimeoutExpired:
            process.kill()
            logger.warning("Application forcefully terminated")
        
        # Final analysis
        if LOG.exists():
            log_content = LOG.read_text(encoding="utf-8", errors="ignore")
            
            logger.info("\n📋 Final Analysis:")
            logger.info(f"FrameTick detected: {frame_tick_detected}")
            logger.info(f"Rendering metrics detected: {rendering_detected}")
            logger.info(f"Total log entries: {len([line for line in log_content.split('\n') if line.strip()])}")
            
            if frame_tick_detected and rendering_detected:
                logger.info("🎉 SUCCESS: Full rendering mode activated!")
                return True
            elif frame_tick_detected:
                logger.info("⚠️  PARTIAL: FrameTick active but no rendering metrics")
                return False
            else:
                logger.info("❌ FAILED: No full rendering detected")
                return False
        else:
            logger.error("No log file generated")
            return False
            
    except Exception as e:
        logger.error(f"Test failed: {e}")
        return False

if __name__ == "__main__":
    success = test_full_rendering_mode()
    if success:
        logger.info("\n🎉 The full show is rendering successfully!")
    else:
        logger.info("\n⚠️  The application needs configuration to show full visuals")