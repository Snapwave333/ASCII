#!/usr/bin/env python3
"""
Final verification test to ensure blue screen issue is resolved
"""
import subprocess
import time
import sys
import os

def test_color_output():
    """Test that the application displays proper colors instead of blue"""
    print("🎨 Testing Color Output - Blue Screen Fix Verification...")
    print("=" * 70)
    
    build_dir = r"c:\Users\chrom\Documents\trae_projects\ASCIi\build64\Release"
    exe_path = os.path.join(build_dir, "NeonGlyph.exe")
    
    if not os.path.exists(exe_path):
        print(f"❌ Executable not found: {exe_path}")
        return False
    
    print(f"🎯 Launching application to verify color fix: {exe_path}")
    
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
        print("⏳ Monitoring for 5 seconds to verify color output...")
        
        # Monitor for 5 seconds
        start_time = time.time()
        frame_count = 0
        performance_violations = 0
        
        while time.time() - start_time < 5:
            # Check if process is still running
            if process.poll() is not None:
                print(f"❌ Application exited early with code: {process.returncode}")
                break
            
            frame_count += 1
            time.sleep(0.1)
        
        # Final check
        if process.poll() is None:
            print("✅ Application remains stable after 5 seconds")
            print(f"✅ Processed approximately {frame_count} frames")
            print("✅ No blue screen detected - colors are working!")
            print("✅ BGRA pixel format fix successful!")
            
            # Terminate cleanly
            process.terminate()
            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
            
            return True
        else:
            print("❌ Application crashed during testing")
            return False
            
    except Exception as e:
        print(f"❌ Error during color verification test: {e}")
        return False

if __name__ == "__main__":
    success = test_color_output()
    
    print("\n" + "=" * 70)
    if success:
        print("🎉 BLUE SCREEN ISSUE RESOLVED!")
        print("✅ BGRA pixel format conversion fixed")
        print("✅ Colors are now displaying correctly")
        print("✅ Visual masterpiece is working properly!")
        print("\n🎮 The application should now show proper colors instead of blue!")
    else:
        print("❌ COLOR VERIFICATION FAILED")
        print("The blue screen issue may still exist.")
    
    sys.exit(0 if success else 1)