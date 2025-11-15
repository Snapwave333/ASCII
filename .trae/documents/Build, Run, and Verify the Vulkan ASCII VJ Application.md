## Preflight Checks
- Confirm Visual Studio (VS 2022) and MSVC toolchain are available; use the VS Developer Command Prompt.
- Ensure Vulkan SDK is installed and `VULKAN_SDK` environment variable is set; verify `vulkan-1.dll` is on PATH.
- Update GPU drivers to the latest version; keep validation layers enabled for first run.

## Build and Launch
- From the VS Developer Command Prompt, run `build.bat` in the project root to configure and build with CMake.
- After a successful build, launch `build\Release\NeonGlyph.exe` (or your configured output).
- Keep system audio playing to drive WASAPI loopback and the MusicAnalyzer; watch for live visuals.

## Runtime Verification
- Confirm audio capture: observe non-zero volume/BPM; log or overlay simple HUD text for `bpm`, `section`, `energy`.
- Confirm AI Director state updates: check transitions between `verse`, `bridge`, `chorus` and pacing/shot changes.
- Confirm Renderer execution: maze/text/wave directives should render on-screen; verify instanced draw count equals ASCII grid size.
- Confirm frame cadence: FPS near target, stutter-free; draw calls remain under budget using instancing.

## Diagnostics (If Issues Occur)
- Swapchain/Surface: if the window is black or present fails, verify surface creation and swapchain setup, render pass, and framebuffers.
- Descriptors/Samplers: ensure atlas image view and sampler bind correctly; verify descriptor writes succeed and are bound before draw.
- Instance Buffer Layout: match instance struct stride/offsets to vertex shader; validate `glyphIndex` mapping to 16×16 atlas tiles.
- Command Recording: begin render pass, bind pipeline/descriptors/buffers, issue a single instanced draw, end render pass.
- Validation Layers: enable and check logs for pipeline state errors, descriptor set binding issues, and synchronization problems.

## Acceptance Criteria
- Audio → MusicAnalyzer → AI Director → Renderer pipeline drives visible ASCII scenes in the Vulkan window.
- DirectorCommands alter visuals coherently in response to music sections and dynamics.
- Performance meets baseline targets (smooth rendering, reasonable CPU/GPU loads).

## Contingency & Iteration
- If GPU path has gaps, temporarily enable console output for visibility while fixing swapchain and graphics pipeline.
- Add targeted logs (instance count, atlas UV tile index, descriptor set success) to isolate failures.
- Iterate on glyph pipeline shaders (UV mapping, brightness/contrast) and instance buffer updates until visuals are correct.

## Next Enhancements
- Beat-synced motion: use `audio_sync.phase` to modulate motion/effects.
- Palette and coloring: map `mise_en_scene.palette` into fragment shader colorization.
- Output integrations: plan Spout/NDI for Phase 4 broadcasting after core visuals are stable.