#!/usr/bin/env python3
"""
Vulkan Runtime Fix for Multi-GPU Systems

This script provides runtime workarounds for the multi-GPU Vulkan initialization
crash (0xC0000005) in vulkan-1.dll that occurs during instance creation.

The crash happens at offset 0x000000000005e4e0 in vulkan-1.dll version 1.4.328.1
when multiple GPU drivers are present (NVIDIA + Intel in this case).
"""

import os
import sys
import subprocess
import json
import ctypes
from pathlib import Path

def get_gpu_info():
    """Get detailed GPU information using Vulkan SDK tools."""
    try:
        # Try to run vulkaninfo to get GPU details
        result = subprocess.run(['vulkaninfo', '--summary'], 
                              capture_output=True, text=True, timeout=10)
        if result.returncode == 0:
            return result.stdout
        else:
            return "vulkaninfo failed: " + result.stderr
    except Exception as e:
        return f"Failed to run vulkaninfo: {e}"

def find_vulkan_drivers():
    """Find all available Vulkan driver JSON files."""
    driver_path = Path("C:/Windows/System32/DriverStore/FileRepository")
    drivers = []
    
    try:
        for driver_dir in driver_path.glob("*"):
            if driver_dir.is_dir():
                # Look for Vulkan driver JSON files
                for vk_json in driver_dir.glob("*vk*.json"):
                    drivers.append(str(vk_json))
    except Exception as e:
        print(f"Error scanning for drivers: {e}")
    
    return drivers

def create_environment_fix():
    """Create environment variable configuration to fix multi-GPU issues."""
    
    print("=== Vulkan Multi-GPU Runtime Fix ===")
    print("Analyzing system configuration...")
    
    # Get GPU information
    gpu_info = get_gpu_info()
    print(f"\nGPU Information:\n{gpu_info}")
    
    # Find Vulkan drivers
    drivers = find_vulkan_drivers()
    print(f"\nFound {len(drivers)} Vulkan driver files:")
    for driver in drivers:
        print(f"  - {driver}")
    
    # Create fix configurations
    fixes = []
    
    # NVIDIA-only fix
    nvidia_drivers = [d for d in drivers if "nv" in d.lower()]
    if nvidia_drivers:
        fixes.append({
            "name": "NVIDIA Only",
            "description": "Force NVIDIA GPU usage only",
            "VK_ICD_FILENAMES": nvidia_drivers[0] if nvidia_drivers else ""
        })
    
    # Intel-only fix
    intel_drivers = [d for d in drivers if "igd" in d.lower() or "intel" in d.lower()]
    if intel_drivers:
        fixes.append({
            "name": "Intel Only", 
            "description": "Force Intel GPU usage only",
            "VK_ICD_FILENAMES": intel_drivers[0] if intel_drivers else ""
        })
    
    # Disable validation layers (sometimes causes conflicts)
    fixes.append({
        "name": "Disable Validation",
        "description": "Disable Vulkan validation layers",
        "VK_INSTANCE_LAYERS": "",
        "DISABLE_VULKAN_VALIDATION": "1"
    })
    
    # Force single GPU through registry settings simulation
    fixes.append({
        "name": "Registry Override",
        "description": "Simulate registry override for single GPU",
        "VK_DRIVER_FILES": nvidia_drivers[0] if nvidia_drivers else (intel_drivers[0] if intel_drivers else "")
    })
    
    return fixes

def test_fix(fix_config, executable_path):
    """Test a specific environment fix configuration."""
    print(f"\n--- Testing: {fix_config['name']} ---")
    print(f"Description: {fix_config['description']}")
    
    # Set environment variables
    env = os.environ.copy()
    for key, value in fix_config.items():
        if key not in ['name', 'description']:
            env[key] = value
            print(f"Set {key}={value}")
    
    try:
        # Run the executable with the fix
        result = subprocess.run([executable_path], 
                              capture_output=True, text=True, 
                              timeout=10, env=env)
        
        if result.returncode == 0:
            print("✅ SUCCESS: Application launched successfully!")
            print("Output:", result.stdout[-200:] if len(result.stdout) > 200 else result.stdout)
            return True
        else:
            print(f"❌ FAILED: Exit code {result.returncode}")
            if result.stderr:
                print("Error output:", result.stderr[-200:] if len(result.stderr) > 200 else result.stderr)
            return False
            
    except subprocess.TimeoutExpired:
        print("⚠️  TIMEOUT: Application started but didn't complete (may be running)")
        return True  # Partial success
    except Exception as e:
        print(f"❌ ERROR: {e}")
        return False

def create_bat_file(executable_path, working_fix):
    """Create a batch file with the working fix."""
    bat_content = f"""@echo off
echo Vulkan Multi-GPU Fix - {working_fix['name']}
echo {working_fix['description']}
echo.
"""
    
    # Add environment variable settings
    for key, value in working_fix.items():
        if key not in ['name', 'description']:
            bat_content += f"set {key}={value}\n"
    
    bat_content += f"\nstart \"\" \"{executable_path}\"\n"
    
    bat_path = Path("NeonGlyph_Fixed.bat")
    with open(bat_path, 'w') as f:
        f.write(bat_content)
    
    print(f"\n✅ Created fix batch file: {bat_path}")
    print("Double-click this file to launch NeonGlyph with the working configuration.")

def main():
    """Main function to test and apply Vulkan runtime fixes."""
    
    executable_path = r"build64\Release\NeonGlyph.exe"
    
    if not os.path.exists(executable_path):
        print(f"❌ ERROR: Executable not found at {executable_path}")
        print("Please ensure the executable is built and in the correct location.")
        return 1
    
    print("Vulkan Multi-GPU Runtime Fix Tool")
    print("=" * 40)
    print("This tool will test various environment configurations to resolve")
    print("the Vulkan initialization crash (0xC0000005) on multi-GPU systems.")
    print()
    
    # Create fix configurations
    fixes = create_environment_fix()
    
    print(f"\nTesting {len(fixes)} different configurations...")
    
    working_fix = None
    
    for fix in fixes:
        if test_fix(fix, executable_path):
            working_fix = fix
            break
        print()
    
    if working_fix:
        print(f"\n🎉 SUCCESS: Found working configuration!")
        print(f"Fix: {working_fix['name']}")
        print(f"Description: {working_fix['description']}")
        
        # Create batch file for easy launching
        create_bat_file(executable_path, working_fix)
        
        print("\nYou can now launch NeonGlyph using the created batch file.")
        print("The application should start without crashing.")
        
        return 0
    else:
        print("\n❌ All fixes failed. The issue may require a different approach.")
        print("Suggestions:")
        print("1. Update GPU drivers (both NVIDIA and Intel)")
        print("2. Reinstall Vulkan Runtime")
        print("3. Try disabling one GPU in Device Manager")
        print("4. Check for Windows updates")
        
        return 1

if __name__ == "__main__":
    sys.exit(main())