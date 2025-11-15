## Environment
- Open a "x64 Native Tools for VS" Developer Command Prompt (Visual Studio 2022)
- Ensure `cmake` and MSVC toolchain are available (`cmake --version`, `cl`)

## Build
- Navigate: `cd c:\Users\chrom\Documents\trae_projects\ASCIi`
- Configure (64-bit): `cmake -S . -B build64 -DCMAKE_BUILD_TYPE=Release -A x64`
- Build: `cmake --build build64 --config Release --target NeonGlyph`
- Artifact: `build64\Release\NeonGlyph.exe`

## Launch & Verify
- Run: `c:\Users\chrom\Documents\trae_projects\ASCIi\build64\Release\NeonGlyph.exe`
- Confirm:
  - Window background initializes to pure black (`#000000`)
  - Console shows ASCII frame metrics and performance stats updating ≥1/sec
  - No critical Vulkan errors (`VK_ERROR_*`) in output; only non-critical warnings allowed
  - Launch completes within 3 seconds

## Configuration Options
- Edit `config\default.json`:
  - `render.startupBgColor`: set RGBA hex (e.g., `"#000000FF"`)
  - `render.blackStartup`: ensure `true` for "music venus" profile

## QA Checklist
- Build completes without compiler errors/warnings
- FPS and frame metrics visible; latency and jitter values update each second
- Validation layers quiet for critical errors

## Troubleshooting
- If CMake generator mismatch: add `-G "Visual Studio 17 2022" -A x64`
- If VS tools missing: run from the Visual Studio Developer Command Prompt (x64)
- If launch shows white background: verify config keys and re-run

## Deliverables
- Build logs, launch output snippet, validation outcome summary (errors/warnings), confirmation of black startup