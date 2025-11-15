#!/usr/bin/env python3
"""
Advanced Vulkan Runtime Fix for Multi-GPU Systems

This script provides advanced workarounds for the specific vulkan-1.dll crash
at offset 0x000000000005e4e0 that occurs during multi-GPU initialization.
"""

import os
import sys
import subprocess
import time
from pathlib import Path

def create_minimal_vulkan_env():
    """Create a minimal Vulkan environment that bypasses problematic initialization."""
    
    # Get system information
    nvidia_64 = r"C:\Windows\System32\DriverStore\FileRepository\nvami.inf_amd64_f6ed7dd5d89ca48a\nv-vk64.json"
    intel_64 = r"C:\Windows\System32\DriverStore\FileRepository\iigd_dch.inf_amd64_38e51a298b862feb\igvk64.json"
    
    # Create minimal environment configurations
    configs = [
        {
            "name": "Ultra-Minimal NVIDIA",
            "env": {
                "VK_ICD_FILENAMES": nvidia_64,
                "VK_INSTANCE_LAYERS": "",  # Completely disable layers
                "VK_LOADER_LAYERS_DISABLE": "1",  # Disable loader layers
                "DISABLE_VULKAN_VALIDATION": "1",
                "VK_KHRONOS_validation": "disable"  # Disable specific validation
            }
        },
        {
            "name": "Ultra-Minimal Intel", 
            "env": {
                "VK_ICD_FILENAMES": intel_64,
                "VK_INSTANCE_LAYERS": "",
                "VK_LOADER_LAYERS_DISABLE": "1",
                "DISABLE_VULKAN_VALIDATION": "1",
                "VK_KHRONOS_validation": "disable"
            }
        },
        {
            "name": "Registry Override + Minimal",
            "env": {
                "VK_ICD_FILENAMES": nvidia_64,
                "VK_DRIVER_FILES": nvidia_64,
                "VK_INSTANCE_LAYERS": "",
                "VK_LOADER_LAYERS_DISABLE": "1",
                "VULKAN_DISABLE_VALIDATION_CACHE": "1"
            }
        },
        {
            "name": "Windows Compatibility Mode",
            "env": {
                "VK_ICD_FILENAMES": nvidia_64,
                "__COMPAT_LAYER": "Win8RTM",  # Windows 8 compatibility
                "VK_INSTANCE_LAYERS": "",
                "PROCESSOR_ARCHITECTURE": "AMD64",  # Force 64-bit mode
                "VK_DISABLE_OPTIMUS": "1"  # Disable NVIDIA Optimus switching
            }
        },
        {
            "name": "Memory Protection Bypass",
            "env": {
                "VK_ICD_FILENAMES": nvidia_64,
                "VK_INSTANCE_LAYERS": "",
                "VULKAN_MEMORY_DEBUG": "1",  # Enable memory debugging
                "VULKAN_DISABLE_MEMORY_POOLS": "1",  # Disable memory pools
                "VK_DISABLE_MEMORY_VALIDATION": "1"
            }
        }
    ]
    
    return configs

def test_with_delay(fix_config, executable_path):
    """Test a fix with delay to allow proper initialization."""
    print(f"\n--- Testing: {fix_config['name']} ---")
    
    # Set environment variables
    env = os.environ.copy()
    for key, value in fix_config['env'].items():
        env[key] = value
        print(f"Set {key}={value}")
    
    try:
        # Start the process
        process = subprocess.Popen([executable_path], 
                                 stdout=subprocess.PIPE, 
                                 stderr=subprocess.PIPE, 
                                 text=True, env=env)
        
        print("Process started, waiting for initialization...")
        
        # Wait a bit for initialization
        time.sleep(3)
        
        # Check if process is still running
        return_code = process.poll()
        
        if return_code is None:
            print("✅ SUCCESS: Process is still running after 3 seconds!")
            # Try to read some output
            try:
                stdout, stderr = process.communicate(timeout=2)
                if stdout:
                    print("Output:", stdout[:500])
                if stderr:
                    print("Errors:", stderr[:500])
            except:
                pass
            
            # Terminate the process
            try:
                process.terminate()
                process.wait(timeout=2)
            except:
                process.kill()
            
            return True
        else:
            print(f"❌ FAILED: Process exited with code {return_code}")
            stdout, stderr = process.communicate()
            if stdout:
                print("Output:", stdout[:500])
            if stderr:
                print("Errors:", stderr[:500])
            return False
            
    except Exception as e:
        print(f"❌ ERROR: {e}")
        return False

def create_advanced_bat_file(executable_path, working_fix):
    """Create an advanced batch file with additional safety measures."""
    
    bat_content = f"""@echo off
echo Advanced Vulkan Multi-GPU Fix - {working_fix['name']}
echo {working_fix['name']}
echo.
echo This batch file applies advanced environment settings to resolve
echo the Vulkan initialization crash on multi-GPU systems.
echo.

:: Set compatibility mode
set __COMPAT_LAYER=Win8RTM

:: Set processor architecture
set PROCESSOR_ARCHITECTURE=AMD64

:: Vulkan-specific fixes
"""
    
    # Add environment variables
    for key, value in working_fix['env'].items():
        bat_content += f"set {key}={value}\n"
    
    bat_content += "\n:: Launch with error handling\necho Launching NeonGlyph with fix applied...\nstart \"\" \"" + executable_path + "\"\n"
    
    bat_path = Path("NeonGlyph_Advanced_Fix.bat")
    with open(bat_path, 'w') as f:
        f.write(bat_content)
    
    print(f"\n✅ Created advanced fix batch file: {bat_path}")
    print("This batch file includes additional Windows compatibility settings.")

def create_registry_fix():
    """Create a registry fix for persistent Vulkan configuration."""
    
    reg_content = """Windows Registry Editor Version 5.00

; Vulkan Multi-GPU Fix Registry Settings
; This forces single GPU usage and disables problematic features

[HKEY_CURRENT_USER\Software\Khronos\Vulkan\Settings]
"VK_ICD_FILENAMES"="C:\\Windows\\System32\\DriverStore\\FileRepository\\nvami.inf_amd64_f6ed7dd5d89ca48a\\nv-vk64.json"
"disable_optimus"=dword:00000001

[HKEY_CURRENT_USER\Software\Khronos\Vulkan\ImplicitLayers]
"disable_all"=dword:00000001

[HKEY_LOCAL_MACHINE\SOFTWARE\Khronos\Vulkan\Settings]
"VK_ICD_FILENAMES"="C:\\Windows\\System32\\DriverStore\\FileRepository\\nvami.inf_amd64_f6ed7dd5d89ca48a\\nv-vk64.json"
"SingleGpuMode"=dword:00000001
"DisableMultiGPU"=dword:00000001
"""
    
    reg_path = Path("vulkan_multigpu_fix.reg")
    with open(reg_path, 'w') as f:
        f.write(reg_content)
    
    print(f"\n✅ Created registry fix file: {reg_path}")
    print("WARNING: Only apply this registry fix if other methods fail!")
    print("To apply: Double-click the .reg file and confirm the changes.")
    print("To undo: Use System Restore or manually remove the registry keys.")

def main():
    """Main function for advanced Vulkan runtime fixes."""
    
    executable_path = r"build64\Release\NeonGlyph.exe"
    
    if not os.path.exists(executable_path):
        print(f"❌ ERROR: Executable not found at {executable_path}")
        return 1
    
    print("Advanced Vulkan Multi-GPU Runtime Fix Tool")
    print("=" * 50)
    print("This tool provides advanced workarounds for the specific")
    print("vulkan-1.dll crash at offset 0x000000000005e4e0")
    print()
    
    # Create advanced configurations
    configs = create_minimal_vulkan_env()
    
    print(f"Testing {len(configs)} advanced configurations...")
    
    working_config = None
    
    for config in configs:
        if test_with_delay(config, executable_path):
            working_config = config
            break
        print()
    
    if working_config:
        print(f"\n🎉 SUCCESS: Found working advanced configuration!")
        print(f"Fix: {working_config['name']}")
        
        # Create advanced batch file
        create_advanced_bat_file(executable_path, working_config)
        
        # Also create registry fix as backup option
        create_registry_fix()
        
        print("\n✅ Solutions created:")
        print("1. Use the batch file for immediate launching")
        print("2. Apply registry fix for permanent system-wide fix (USE WITH CAUTION)")
        print("3. Set the environment variables manually in your development environment")
        
        print(f"\n📝 Manual environment setup:")
        for key, value in working_config['env'].items():
            print(f"set {key}={value}")
        
        return 0
    else:
        print("\n❌ All advanced fixes failed.")
        print("\nFinal recommendations:")
        print("1. Update GPU drivers to latest versions")
        print("2. Reinstall Vulkan Runtime from LunarG")
        print("3. Temporarily disable Intel GPU in Device Manager")
        print("4. Use DDU (Display Driver Uninstaller) to clean GPU drivers")
        print("5. Consider using a different Vulkan loader version")
        
        return 1

if __name__ == "__main__":
    sys.exit(main())