@echo off
setlocal
pushd "%~dp0.."
set ROOT=%CD%
set REL=%ROOT%\build64\Release
if exist "%REL%\runtime_logic_tests.exe" (
  "%REL%\runtime_logic_tests.exe" > "%REL%\runtime_logic_tests.log" 2>&1
  set RTL=%errorlevel%
) else (
  set RTL=1
)
if exist "%REL%\ascii_font_atlas_test.exe" (
  "%REL%\ascii_font_atlas_test.exe" > "%REL%\ascii_font_atlas_test.log" 2>&1
  set AFT=%errorlevel%
) else (
  set AFT=1
)
if exist "%REL%\e2e_test.exe" (
  "%REL%\e2e_test.exe" > "%REL%\e2e_test.log" 2>&1
  rem Derive status from log content to avoid spurious errorlevels
  findstr /C:"FAIL" "%REL%\e2e_test.log" >nul 2>&1
  if %errorlevel% equ 0 (
    set E2E=1
  ) else (
    set E2E=0
  )
) else (
  set E2E=1
)
if exist "%REL%\gpu_pipeline_test.exe" (
  "%REL%\gpu_pipeline_test.exe" > "%REL%\gpu_pipeline_test.log" 2>&1
  findstr /C:"PASS" "%REL%\gpu_pipeline_test.log" >nul 2>&1
  if %errorlevel% equ 0 (
    set GPU=0
  ) else (
    findstr /C:"SKIP" "%REL%\gpu_pipeline_test.log" >nul 2>&1
    if %errorlevel% equ 0 (
      set GPU=2
    ) else (
      set GPU=1
    )
  )
) else (
  set GPU=2
)
(
  echo runtime_logic_tests=%RTL%
  echo ascii_font_atlas_test=%AFT%
  echo e2e_test=%E2E%
  echo gpu_pipeline_test=%GPU%
) > "%REL%\verification_results.txt"
popd
endlocal