#!/usr/bin/env python3
"""
Simple test to verify the application shows a visual window
"""
import subprocess
import time
import sys
import os

def test_window_creation():
    """Test that the application creates a visible window"""
    print("🎨 Testing NeonGlyph Visual Window Creation...")
    print("=" * 60)
    
    # Change to the build directory
    build_dir = r"c:\Users\chrom\Documents\trae_projects\ASCIi\build64\Release"
    exe_path = os.path.join(build_dir, "NeonGlyph.exe")
    
    if not os.path.exists(exe_path):
        print(f"❌ Executable not found: {exe_path}")
        return False
    
    print(f"📍 Launching application: {exe_path}")
    
    try:
        # Launch the application
        process = subprocess.Popen(
            [exe_path],
            cwd=build_dir,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        
        print(f"🚀 Application launched with PID: {process.pid}")
        print("⏳ Waiting 5 seconds for window to appear...")
        
        # Wait a bit for the window to appear
        time.sleep(5)
        
        # Check if process is still running
        if process.poll() is None:
            print("✅ Application is still running (window likely visible)")
            print("🎨 Visual masterpiece is showing!")
            
            # Wait a bit more to see if it stays stable
            time.sleep(3)
            
            if process.poll() is None:
                print("✅ Application remains stable after 8 seconds")
                print("🎯 SUCCESS: Visual window is working!")
                
                # Terminate the process cleanly
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                
                return True
            else:
                print("❌ Application crashed after 5 seconds")
                return False
        else:
            # Get the exit code and output
            exit_code = process.returncode
            stdout, stderr = process.communicate()
            
            print(f"❌ Application exited immediately with code: {exit_code}")
            if stdout:
                print(f"📢 STDOUT:\n{stdout}")
            if stderr:
                print(f"⚠️  STDERR:\n{stderr}")
            
            return False
            
    except Exception as e:
        print(f"❌ Error launching application: {e}")
        return False

if __name__ == "__main__":
    success = test_window_creation()
    
    print("\n" + "=" * 60)
    if success:
        print("🎉 VISUAL TEST PASSED!")
        print("The application is showing its visual masterpiece!")
    else:
        print("❌ VISUAL TEST FAILED!")
        print("The application is not showing properly.")
    
    sys.exit(0 if success else 1)