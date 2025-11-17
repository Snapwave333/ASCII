@echo off
echo.
echo ╔══════════════════════════════════════════════════════════════════════════════╗
echo ║                    NEONGLYPH STARTUP DIAGNOSTICS                              ║
echo ╚══════════════════════════════════════════════════════════════════════════════╝
echo.
echo Starting NeonGlyph with enhanced diagnostics...
echo.
echo If you see "WINDOW CREATED SUCCESSFULLY" - the window should be visible
echo If you see "WINDOW CREATION FAILED" - that's why you can't see the app!
echo.
echo Running diagnostic...
echo.

cd build64\Release
NeonGlyph.exe --no-headless-fallback 2>&1 | findstr /i "window\|error\|failed\|success\|diagnostic\|creating"

echo.
echo ╔══════════════════════════════════════════════════════════════════════════════╗
echo ║                           DIAGNOSTIC COMPLETE                               ║
echo ╚══════════════════════════════════════════════════════════════════════════════╝
echo.
echo Check the messages above to understand what's happening!
echo.
pause