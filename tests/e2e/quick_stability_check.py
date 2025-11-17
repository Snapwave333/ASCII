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

def quick_stability_check():
    """Quick stability check to identify crash patterns"""
    logger.info("🔍 Quick Stability Check for NeonGlyph")
    logger.info("=" * 40)
    
    # Check existing log for crash patterns
    if LOG.exists():
        log_content = LOG.read_text(encoding="utf-8", errors="ignore")
        lines = log_content.split('\n')
        
        # Count startup sequences
        startup_count = log_content.count("Version 3.0.1 Startup")
        vulkan_count = log_content.count("CreateVulkanSurfaceMs")
        rendering_count = log_content.count("RenderMs") + log_content.count("FrameMs")
        
        logger.info(f"Startup sequences detected: {startup_count}")
        logger.info(f"Vulkan surface creations: {vulkan_count}")
        logger.info(f"Rendering operations: {rendering_count}")
        
        # Calculate crash indicators
        if startup_count > vulkan_count:
            logger.warning(f"⚠️  Potential crashes: {startup_count - vulkan_count} startups without Vulkan completion")
        
        if vulkan_count > rendering_count:
            logger.warning(f"⚠️  Rendering issues: {vulkan_count - rendering_count} Vulkan surfaces without rendering")
        
        # Check last sequence for completeness
        if startup_count > 0:
            last_startup = log_content.rfind("Version 3.0.1 Startup")
            last_lines = log_content[last_startup:].split('\n')[-10:]  # Last 10 lines
            
            logger.info("Last startup sequence (final 10 lines):")
            for i, line in enumerate(last_lines, 1):
                if line.strip():
                    logger.info(f"  {i}: {line.strip()}")
    
    # Test single run
    logger.info("\n🧪 Testing single application run...")
    
    # Clear previous log
    if LOG.exists():
        LOG.unlink()
    
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
        
        logger.info(f"Process started with PID: {process.pid}")
        
        # Monitor for 10 seconds
        for i in range(10):
            time.sleep(1)
            return_code = process.poll()
            
            if return_code is not None:
                logger.info(f"Process exited after {i+1} seconds with code: {return_code}")
                
                # Check stderr for crash information
                stderr_output = process.stderr.read().decode('utf-8', errors='ignore')
                if stderr_output:
                    logger.error("STDERR output:")
                    logger.error(stderr_output)
                
                # Analyze log
                if LOG.exists():
                    log_content = LOG.read_text(encoding="utf-8", errors="ignore")
                    has_startup = "Startup Window+GLFW initialized" in log_content
                    has_vulkan = "CreateVulkanSurfaceMs" in log_content
                    has_rendering = "RenderMs" in log_content or "FrameMs" in log_content
                    
                    logger.info(f"Log analysis:")
                    logger.info(f"  Startup completed: {has_startup}")
                    logger.info(f"  Vulkan initialized: {has_vulkan}")
                    logger.info(f"  Rendering active: {has_rendering}")
                    
                    if return_code != 0:
                        logger.error(f"❌ CRASH DETECTED - Exit code: {return_code}")
                        logger.error(f"  Progress: {sum([has_startup, has_vulkan, has_rendering])}/3 stages")
                        
                        # Show last few log lines
                        lines = log_content.split('\n')
                        logger.error("Last 5 log lines before crash:")
                        for line in lines[-5:]:
                            if line.strip():
                                logger.error(f"  {line.strip()}")
                
                break
            else:
                logger.info(f"Still running after {i+1} seconds...")
        
        else:
            # Still running after 10 seconds
            logger.info("✅ Application stable after 10 seconds")
            process.terminate()
            try:
                process.wait(timeout=5)
                logger.info("Application terminated gracefully")
            except subprocess.TimeoutExpired:
                process.kill()
                logger.warning("Application had to be forcefully terminated")
        
        return True
        
    except Exception as e:
        logger.error(f"❌ Failed to start application: {e}")
        return False

if __name__ == "__main__":
    quick_stability_check()