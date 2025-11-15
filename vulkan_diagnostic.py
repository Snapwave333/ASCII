#!/usr/bin/env python3
"""
NeonGlyph Vulkan Diagnostic Tool
Comprehensive diagnostic script to identify Vulkan runtime issues
"""

import subprocess
import sys
import os
import json
import platform
import ctypes
from pathlib import Path

def run_command(cmd, description):
    """Run a command and capture output"""
    print(f"\n=== {description} ===")
    try:
        result = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=30)
        print(f"Command: {cmd}")
        print(f"Exit Code: {result.returncode}")
        if result.stdout:
            print(f"STDOUT:\n{result.stdout}")
        if result.stderr:
            print(f"STDERR:\n{result.stderr}")
        return result.returncode, result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        print("Command timed out after 30 seconds")
        return -1, "", "Timeout"
    except Exception as e:
        print(f"Error running command: {e}")
        return -1, "", str(e)

def check_vulkan_runtime():
    """Check Vulkan runtime availability"""
    print("\n" + "="*60)
    print("NEONGLYPH VULKAN DIAGNOSTIC REPORT")
    print("="*60)
    
    # System information
    print(f"\nSystem Information:")
    print(f"Platform: {platform.system()}")
    print(f"Architecture: {platform.architecture()}")
    print(f"Machine: {platform.machine()}")
    print(f"Processor: {platform.processor()}")
    print(f"Python Version: {sys.version}")
    
    # Check Vulkan DLL
    print(f"\nVulkan Runtime Check:")
    vulkan_dlls = ["vulkan-1.dll", "vulkan.dll"]
    vulkan_found = False
    
    for dll in vulkan_dlls:
        try:
            # Try to load the DLL
            ctypes.windll.LoadLibrary(dll)
            print(f"✓ {dll} - Found and loadable")
            vulkan_found = True
            break
        except:
            print(f"✗ {dll} - Not found or not loadable")
    
    if not vulkan_found:
        print("⚠ Vulkan runtime not detected in system path")
    
    # Check environment variables
    print(f"\nEnvironment Variables:")
    vulkan_vars = ["VULKAN_SDK", "VK_LAYER_PATH", "VK_INSTANCE_LAYERS", "VK_LOADER_DEBUG"]
    for var in vulkan_vars:
        value = os.environ.get(var, "Not set")
        print(f"{var}: {value}")
    
    return vulkan_found

def test_vulkan_instance_creation():
    """Test basic Vulkan instance creation"""
    print(f"\nVulkan Instance Creation Test:")
    
    # Test with validation layers
    env = os.environ.copy()
    env["VK_INSTANCE_LAYERS"] = "VK_LAYER_KHRONOS_validation"
    env["VK_LOADER_DEBUG"] = "all"
    
    cmd = f'cd "{os.getcwd()}" && set VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation && set VK_LOADER_DEBUG=all && echo "Testing Vulkan instance creation..."'
    
    returncode, stdout, stderr = run_command(cmd, "Vulkan Environment Test")
    
    if returncode == 0:
        print("✓ Vulkan environment test passed")
    else:
        print("✗ Vulkan environment test failed")
    
    return returncode == 0

def test_graphics_drivers():
    """Test graphics driver information"""
    print(f"\nGraphics Driver Information:")
    
    # Try to get GPU information using wmic
    cmd = "wmic path win32_VideoController get name, driverversion, adapterram"
    returncode, stdout, stderr = run_command(cmd, "Graphics Driver Info")
    
    if returncode == 0 and stdout:
        print("✓ Graphics driver information retrieved")
        # Parse driver information
        lines = stdout.strip().split('\n')
        for line in lines[1:]:  # Skip header
            if line.strip():
                print(f"  {line.strip()}")
    else:
        print("✗ Could not retrieve graphics driver information")
    
    return returncode == 0

def test_vulkan_sdk_tools():
    """Test Vulkan SDK tools availability"""
    print(f"\nVulkan SDK Tools Check:")
    
    vulkan_tools = [
        "vulkaninfoSDK.exe",
        "vkcube.exe", 
        "vkconfig.exe"
    ]
    
    tools_found = 0
    for tool in vulkan_tools:
        result = subprocess.run(f"where {tool}", shell=True, capture_output=True)
        if result.returncode == 0:
            print(f"✓ {tool} - Found")
            tools_found += 1
        else:
            print(f"✗ {tool} - Not found")
    
    if tools_found > 0:
        print(f"✓ {tools_found}/{len(vulkan_tools)} Vulkan SDK tools available")
    else:
        print("⚠ No Vulkan SDK tools found")
    
    return tools_found > 0

def test_neonglyph_executable():
    """Test NeonGlyph executable"""
    print(f"\nNeonGlyph Executable Test:")
    
    executable_path = Path("build64/Release/NeonGlyph.exe")
    if not executable_path.exists():
        print("✗ NeonGlyph executable not found")
        return False
    
    print(f"✓ Executable found: {executable_path}")
    print(f"  Size: {executable_path.stat().st_size} bytes")
    print(f"  Modified: {executable_path.stat().st_mtime}")
    
    # Test basic launch with timeout
    env = os.environ.copy()
    env["NG_LOG_ONLY"] = "1"
    env["NG_METRICS_PATH"] = "diagnostic_test.log"
    
    cmd = f'"{executable_path}"'
    print(f"\nTesting executable launch...")
    
    try:
        result = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=5, env=env)
        print(f"Exit Code: {result.returncode}")
        
        if result.returncode == -1073741819:
            print("✗ CONFIRMED: Access violation (-1073741819)")
            print("  This indicates a memory access violation during startup")
        elif result.returncode == 0:
            print("✓ Executable launched successfully")
        else:
            print(f"⚠ Executable exited with code: {result.returncode}")
        
        # Check for log file
        log_file = Path("diagnostic_test.log")
        if log_file.exists():
            print(f"✓ Log file created: {log_file}")
            with open(log_file) as f:
                log_content = f.read()
                if log_content:
                    print(f"Log content preview: {log_content[:200]}...")
        
        return result.returncode != -1073741819
        
    except subprocess.TimeoutExpired:
        print("⚠ Executable launched but did not exit within 5 seconds")
        return True  # This might indicate it's running
    except Exception as e:
        print(f"✗ Error testing executable: {e}")
        return False

def generate_recommendations():
    """Generate recommendations based on diagnostic results"""
    print(f"\n" + "="*60)
    print("RECOMMENDATIONS FOR RESOLVING VULKAN CRASH")
    print("="*60)
    
    recommendations = [
        "1. UPDATE GRAPHICS DRIVERS:",
        "   - Download latest drivers from GPU manufacturer website",
        "   - Perform clean installation to avoid conflicts",
        "   - Ensure Vulkan API support (version 1.2+ required)",
        "",
        "2. INSTALL/UPDATE VULKAN SDK:",
        "   - Download latest Vulkan SDK from LunarG",
        "   - Install runtime components",
        "   - Verify VK_LAYER_KHRONOS_validation is available",
        "",
        "3. SYSTEM DIAGNOSTICS:",
        "   - Run Windows Memory Diagnostic",
        "   - Check Event Viewer for GPU-related errors",
        "   - Verify DirectX runtime is up to date",
        "",
        "4. APPLICATION-SPECIFIC FIXES:",
        "   - Try running with different GPU (if multiple available)",
        "   - Disable GPU acceleration temporarily",
        "   - Run in compatibility mode",
        "",
        "5. DEBUGGING STEPS:",
        "   - Build application in Debug configuration",
        "   - Attach debugger to capture exact crash location",
        "   - Enable Vulkan validation layers for detailed errors"
    ]
    
    for rec in recommendations:
        print(rec)

def main():
    """Main diagnostic function"""
    print("Starting comprehensive Vulkan diagnostic...")
    
    # Run all diagnostic tests
    vulkan_ok = check_vulkan_runtime()
    instance_ok = test_vulkan_instance_creation()
    drivers_ok = test_graphics_drivers()
    tools_ok = test_vulkan_sdk_tools()
    executable_ok = test_neonglyph_executable()
    
    # Summary
    print(f"\n" + "="*60)
    print("DIAGNOSTIC SUMMARY")
    print("="*60)
    
    tests = [
        ("Vulkan Runtime", vulkan_ok),
        ("Instance Creation", instance_ok),
        ("Graphics Drivers", drivers_ok),
        ("Vulkan SDK Tools", tools_ok),
        ("NeonGlyph Executable", executable_ok)
    ]
    
    passed = sum(1 for _, result in tests if result)
    total = len(tests)
    
    for test_name, result in tests:
        status = "✓ PASS" if result else "✗ FAIL"
        print(f"{test_name}: {status}")
    
    print(f"\nOverall: {passed}/{total} tests passed")
    
    if passed == total:
        print("✓ All diagnostic tests passed - issue may be application-specific")
    else:
        print("⚠ Some tests failed - follow recommendations below")
    
    generate_recommendations()
    
    # Save diagnostic report
    print(f"\nDiagnostic report saved to: vulkan_diagnostic_report.txt")

if __name__ == "__main__":
    main()