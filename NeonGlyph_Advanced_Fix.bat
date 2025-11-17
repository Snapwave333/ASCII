@echo off
echo Advanced Vulkan Multi-GPU Fix - Ultra-Minimal NVIDIA
echo Ultra-Minimal NVIDIA
echo.
echo This batch file applies advanced environment settings to resolve
echo the Vulkan initialization crash on multi-GPU systems.
echo.

:: Set compatibility mode
set __COMPAT_LAYER=Win8RTM

:: Set processor architecture
set PROCESSOR_ARCHITECTURE=AMD64

:: Vulkan-specific fixes
set VK_ICD_FILENAMES=C:\Windows\System32\DriverStore\FileRepository\nvami.inf_amd64_f6ed7dd5d89ca48a\nv-vk64.json
set VK_INSTANCE_LAYERS=
set VK_LOADER_LAYERS_DISABLE=1
set DISABLE_VULKAN_VALIDATION=1
set VK_KHRONOS_validation=disable

:: Launch with error handling
echo Launching NeonGlyph with fix applied...
start "" "build64\Release\NeonGlyph.exe"
