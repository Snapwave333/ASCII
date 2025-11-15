@echo off
echo Running NeonGlyph with enhanced error logging...
echo.

REM Set environment variables for enhanced error reporting
set NEONGLYPH_DEBUG=1
set VULKAN_DEBUG=1
set ENABLE_VALIDATION=1

REM Run the application and capture all output
Release\NeonGlyph.exe --verbose --debug --log-level debug > runtime_error_output.log 2>&1

REM Check if the executable exists
if not exist Release\NeonGlyph.exe (
    echo ERROR: NeonGlyph.exe not found in Release folder
    echo Looking for executable in other locations...
    dir /s /b NeonGlyph.exe
    exit /b 1
)

echo.
echo Application finished. Checking for errors...
echo.

REM Search for specific error patterns
findstr /i "stof\|invalid.*argument\|fatal.*error\|exception\|error" runtime_error_output.log

REM Show the last few lines of output
echo.
echo Last 20 lines of output:
tail -20 runtime_error_output.log 2>nul || type runtime_error_output.log | tail -20 2>nul || echo Could not show last lines

echo.
echo Full output saved to runtime_error_output.log