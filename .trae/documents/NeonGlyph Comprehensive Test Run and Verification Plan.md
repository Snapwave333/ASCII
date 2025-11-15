## Overview
- Execute the full performance run, log validation, color-mode verification, integration build, and unit test coverage for NeonGlyph on Windows PowerShell.
- Use project-root paths to keep artifacts together; ensure environment variables affect the launched process immediately.
- Perform clean shutdown after 60 seconds via window close, falling back safely if required.

## Prep
- Confirm key files and flags:
  - `NeonGlyph.exe` present in `build64\Release` (found).
  - Color mode logging and metrics fields implemented in `src/Application.cpp:368-381,568-571,589-593`.
  - `NG_LOG_ONLY` read in `src/Application.cpp:611-613`; `NG_METRICS_PATH` read in `src/Logger.cpp:30-33`.
  - Validation script `validate_color_modes.py` expects `staging_run.log` at current dir.
- Open one PowerShell terminal in project root `c:\Users\chrom\Documents\trae_projects\ASCIi`.

## 1) Performance Run (60s)
- Commands (PowerShell):
  - `cd c:\Users\chrom\Documents\trae_projects\ASCIi`
  - `setx NG_LOG_ONLY 1`
  - `setx NG_METRICS_PATH "$PWD\staging_run.log"`
  - `$env:NG_LOG_ONLY = "1"`
  - `$env:NG_METRICS_PATH = "$PWD\staging_run.log"`
  - `cd build64\Release`
  - `$p = Start-Process -FilePath .\NeonGlyph.exe -PassThru`
  - `Start-Sleep -Seconds 60`
  - `if (-not $p.HasExited) { $null = $p.CloseMainWindow(); $p.WaitForExit(10000); if (-not $p.HasExited) { Stop-Process -Id $p.Id } }`
- Rationale:
  - Immediate environment variables ensure the child process inherits the values (SetX persists, env: applies now).
  - Clean exit attempted via `CloseMainWindow()`; force only if unresponsive.

## 2) Consolidate Logs for Validation
- Copy unified log to project root (needed because `src/Application.cpp:53-55` writes `staging_run.log` relative to the working directory used to launch):
  - `cd c:\Users\chrom\Documents\trae_projects\ASCIi`
  - `Copy-Item -Force .\build64\Release\staging_run.log .\staging_run.log`

## 3) Log File Validation
- Quick checks on `staging_run.log`:
  - `Select-String -Path .\staging_run.log -Pattern "TS=|RenderMs=|PresentMs=|FrameMs=|Var=|Jitter=|DroppedFrames="`
  - `Select-String -Path .\staging_run.log -Pattern "^FrameTimeAvgMs="`
  - `Select-String -Path .\staging_run.log -Pattern "^ColorMode=(mono|truecolor|ansi)$"`
  - `Select-String -Path .\staging_run.log -Pattern "Window (Create|PollEvents|ShouldClose|CreateVulkanSurface)"`
- Expected sources:
  - Per-frame metrics `TS, RenderMs, PresentMs, FrameMs, Var, Jitter, DroppedFrames` from `src/Application.cpp:368-381`.
  - `FrameTimeAvgMs` once/second from `src/Application.cpp:581-593`.
  - Color mode lines from `src/Application.cpp:568-571`.
  - Window timings via `Logger::LogEvent/LogLine` from `src/Window.cpp:33-136,109-121`.

## 4) Color Mode Validation
- From project root:
  - `python validate_color_modes.py > color_validation_report.txt`
  - `if ($LASTEXITCODE -ne 0) { Write-Host "Color validation FAILED"; $LASTEXITCODE }
    else { Write-Host "Color validation OK" }`
- Script behavior (`validate_color_modes.py:27-56`):
  - Segments keyed by `ColorMode=`; asserts mono/truecolor/ansi presence, no ANSI in mono; truecolor/ansi sequences present.

## 5) Integration Build
- Configure and build with optional components:
  - `cmake -S . -B build64_int -A x64 -DWINDOWS_INTEGRATION=ON -DBUILD_STORY_DEMO=ON *> build_integration.log`
  - `cmake --build build64_int --config Release *> build_integration.log`
- Flags validated in `CMakeLists.txt:59-72,81-108`.

## 6) Unit Tests + Coverage
- From project root:
  - `python -m coverage run -m pytest > unit_test_results.txt`
  - `python -m coverage report -m > coverage_report.txt`

## 7) Success Criteria Checks
- Performance:
  - `Select-String .\staging_run.log -Pattern "FrameMs="` and visually inspect stability; compute jitter from adjacent `FrameMs` lines; confirm `DroppedFrames=0` occurrences.
- Error Handling:
  - `Select-String .\staging_run.log -Pattern "Exception|Error"` should show none; allow warnings from optional modules.
- Color Validation:
  - Ensure `color_validation_report.txt` includes three modes and non-zero sequence counts.
- Stability:
  - Startup sequence present: system init and morph/test-card events from `src/Application.cpp:257-279`.
  - Test card display/transition implied by `RenderTestCard` submissions.

## Artifacts Produced
- `staging_run.log` (project root)
- `color_validation_report.txt`
- `build_integration.log`
- `unit_test_results.txt`
- `coverage_report.txt`

## Notes
- If color validation fails due to missing segments, re-check that the 60s run covered at least 8 seconds to cycle `mono → truecolor → ansi` (`src/Application.cpp:552-579` uses 4s per mode). A 60s window suffices.
- If the app starts headless (GLFW disabled), window-event timing logs may be limited; current build defaults enable GLFW (`CMakeLists.txt:126-129`).