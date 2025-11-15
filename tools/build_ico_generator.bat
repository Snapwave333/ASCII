@echo off
echo Building ICO Generator...

REM Try to find a C++ compiler
where cl >nul 2>nul
if %errorlevel% == 0 (
    echo Using Visual Studio compiler...
    cl /EHsc /O2 generate_ico.cpp /Fe:generate_ico.exe
    if %errorlevel% == 0 (
        echo Running ICO generator...
        generate_ico.exe
        if exist neonglyph-icon.ico (
            echo Moving ICO files to resources directory...
            move /Y neonglyph-icon.ico ..\resources\
            move /Y neonglyph-tray-icon.ico ..\resources\
            echo ICO files generated and moved successfully!
        ) else (
            echo ICO generation failed!
        )
    ) else (
        echo Compilation failed!
    )
    goto :end
)

where g++ >nul 2>nul
if %errorlevel% == 0 (
    echo Using GCC compiler...
    g++ -O2 -std=c++17 generate_ico.cpp -o generate_ico.exe
    if %errorlevel% == 0 (
        echo Running ICO generator...
        generate_ico.exe
        if exist neonglyph-icon.ico (
            echo Moving ICO files to resources directory...
            move /Y neonglyph-icon.ico ..\resources\
            move /Y neonglyph-tray-icon.ico ..\resources\
            echo ICO files generated and moved successfully!
        ) else (
            echo ICO generation failed!
        )
    ) else (
        echo Compilation failed!
    )
    goto :end
)

where clang++ >nul 2>nul
if %errorlevel% == 0 (
    echo Using Clang compiler...
    clang++ -O2 -std=c++17 generate_ico.cpp -o generate_ico.exe
    if %errorlevel% == 0 (
        echo Running ICO generator...
        generate_ico.exe
        if exist neonglyph-icon.ico (
            echo Moving ICO files to resources directory...
            move /Y neonglyph-icon.ico ..\resources\
            move /Y neonglyph-tray-icon.ico ..\resources\
            echo ICO files generated and moved successfully!
        ) else (
            echo ICO generation failed!
        )
    ) else (
        echo Compilation failed!
    )
    goto :end
)

echo No C++ compiler found! Please install Visual Studio, MinGW, or Clang.
echo Alternatively, you can use the pre-generated ICO files from the assets directory.

:end
pause