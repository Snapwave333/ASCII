## Runtime Environment Configuration
- Set `VULKAN_SDK`:
  - PowerShell (current session): `Set-Item -Path Env:VULKAN_SDK -Value "C:\VulkanSDK\1.4.328.1"`
  - Persist: `setx VULKAN_SDK "C:\VulkanSDK\1.4.328.1"`
- Verify PATH contains Vulkan tools/binaries:
  - Check: `Get-Command vulkaninfo` (optional) and ensure `C:\VulkanSDK\1.4.328.1\Bin` is discoverable if needed.
  - If missing, temporarily prepend: `$env:PATH = "C:\VulkanSDK\1.4.328.1\Bin;" + $env:PATH`
- Enable ANSI output:
  - Preferred: run in Windows Terminal/PowerShell 7+ (VT enabled by default).
  - If needed, enable VT: `reg add HKCU\Console /v VirtualTerminalLevel /t REG_DWORD /d 1 /f` and restart terminal.
  - Smoke test: `Write-Host "`e[38;2;255;0;0mRED`e[0m"` should print red text.
- Dependencies check:
  - Confirm `vulkan-1.dll` loads (either system or SDK Bin). Use `Get-Item "$env:VULKAN_SDK\Bin\vulkan-1.dll"`.
  - Confirm `glfw3.dll` not required at runtime (link is static); if dynamic, ensure it’s in PATH.

## Application Launch and Validation
- Launch executable:
  - `Start-Process -FilePath .\build64\Release\NeonGlyph.exe -WorkingDirectory .\build64\Release -NoNewWindow`
- Startup cinematic morph sequence validation:
  - Frame timing: measure with Windows Performance Counters or internal logs; target average frame time ≤ 16.7 ms (60 FPS) and stable variance.
  - Rendering quality: confirm ASCII output legibility, symmetry modes (`vertical_mirror`/`radial_4`) apply correctly, no artifacts.
  - Animation interpolation: observe morph techniques (particle → fold → beam → panel → glitch → portal) advancing smooth at ~1s cadence.
- Test card display functionality:
  - Rendering pipeline: verify luminance ramp (`" .:-=+*#%@"`), grid lines, motion check zone, and panel test sections are present.
  - Color modes: in truecolor/ANSI, confirm per-cell color output; in mono, confirm density/contrast separation without color codes.
  - Texture/memory: if FreeType present, confirm font atlas usage and no leaks; if disabled, ensure ASCII-only path remains correct.
  - Display scaling: resize console and confirm canvas adapts without distortion; aspect ratio consistent across sizes.

## Optional Component Reintegration
- Gate Windows integration features behind CMake option:
  - Configure: `cmake -S . -B build64 -A x64 -DWINDOWS_INTEGRATION=ON`
  - Module-by-module validation: rebuild and run; fix compile issues (macro misuse, header consistency), then re-test.
- Reactivate StoryDemo:
  - Configure: `cmake -S . -B build64 -A x64 -DBUILD_STORY_DEMO=ON`
  - Isolate broken modules (e.g., `SeamlessWindow`, `WindowsIntegration`), repair, and re-validate.
- Refactor `WindowsIntegration`/`SeamlessWindow`:
  - Replace constant-as-function macro misuse with proper functions or `__noop`.
  - Unify includes (`GLFW_EXPOSE_NATIVE_WIN32` + `glfw3native.h` where needed); abstract platform interfaces and avoid direct macro coupling.

## Documentation Requirements
- Build Environment Checklist:
  - Visual Studio 2022 with full C++ workload installed.
  - Developer Command Prompt (`VsDevCmd.bat`) available.
  - CMake ≥ 3.20 (system-wide or Python-provided with PATH update).
  - `VULKAN_SDK` environment variable set and verified.
- Build Script Specifications:
  - `scripts/setup_build_env.ps1` performs validation, configures `build64` with `-A x64`, prints clear error messages, and exits non-zero on failures with remediation hints.

## Success Criteria Verification
- Compilation:
  - C++ e2e and primary targets compile without errors; warnings acceptable only if non-critical.
  - x64 target fully built; win32 not used.
- Deployment:
  - Staging unblocked; `NeonGlyph.exe` runs with startup morph and test card functioning.
  - Core features (ASCII rendering, symmetry, panelization, color modes) validated.
- Environment:
  - Build environment documented; setup is scripted and repeatable.
  - Configuration validated for future compilation tasks.

## Monitoring, Logging, and Rollback
- Logging: capture console output to file during validation: `NeonGlyph.exe *> .\staging_run.log`.
- Monitoring: record frame time averages and any error messages; verify no shader or pipeline errors.
- Rollback: if staging validation fails, revert to last known good build and disable optional components via CMake flags until fixed.

## Execution Plan Summary
- Configure environment (Vulkan SDK, ANSI, PATH) and smoke test ANSI.
- Launch `NeonGlyph.exe`, validate morph sequence timing/quality and test card completeness.
- Incrementally re-enable optional modules using CMake flags and repair issues.
- Document environment and script behavior; confirm success criteria and prepare for deployment.

Please confirm to proceed with executing these steps in sequence and performing the staging validation run with logs and metrics.