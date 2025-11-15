@echo off
echo PROJECT NEON-GLYPH Build System
echo ==============================

REM Check for Visual Studio
where cl >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Visual Studio compiler not found!
    echo Please run this script from a Visual Studio Developer Command Prompt
    exit /b 1
)

REM Check for CMake
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: CMake not found!
    echo Please install CMake and add it to your PATH
    exit /b 1
)

REM Check for Vulkan SDK
if not defined VULKAN_SDK (
    echo ERROR: Vulkan SDK not found!
    echo Please install Vulkan SDK and set VULKAN_SDK environment variable
    exit /b 1
)

REM Create build directory
if not exist build64 mkdir build64
cd build64

REM Configure with CMake
echo Configuring project...
cmake -G "Visual Studio 17 2022" -A x64 ..
if %errorlevel% neq 0 (
    echo ERROR: CMake configuration failed!
    exit /b 1
)

REM Build the project
echo Building project...
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo ERROR: Build failed!
    exit /b 1
)

echo.
echo Build completed successfully!
echo Executable: build64\Release\NeonGlyph.exe
echo.
echo To run the application:
echo   build64\Release\NeonGlyph.exe

REM Return to root directory
cd ..
 
REM Update NEXT_STEPS progress automatically
where python >nul 2>&1
if %errorlevel% equ 0 (
    python tools\update_next_steps.py
)
if exist tools\run_verification_suite.bat (
    call tools\run_verification_suite.bat
    where python >nul 2>&1
    if %errorlevel% equ 0 (
        python tools\update_next_steps.py
    )
)