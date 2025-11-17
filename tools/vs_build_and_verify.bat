@echo off
setlocal
set VS_DEV_CMD="C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
if not exist %VS_DEV_CMD% set VS_DEV_CMD="C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"
call %VS_DEV_CMD%
if %errorlevel% neq 0 (
  echo [ERROR] VsDevCmd failed
  exit /b 1
)
set VULKAN_SDK=C:\VulkanSDK\1.4.328.1
cd /d "C:\Users\chrom\Documents\trae_projects\ASCIi"
call .\build.bat
if %errorlevel% neq 0 (
  echo [WARN] Build failed, attempting targeted test build
  cmake --build build64 --config Release --target runtime_logic_tests ascii_font_atlas_test e2e_test gpu_pipeline_test
)
call tools\run_verification_suite.bat
python tools\update_next_steps.py
exit /b 0
