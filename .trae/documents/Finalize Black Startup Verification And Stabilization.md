## Visual Verification
- Relaunch NeonGlyph and confirm window background is pure black at startup
- Toggle palettes (F2–F5) and verify ASCII output overlays correctly on black
- Confirm metrics (RenderMs/PresentMs/FrameMs/Jitter/DroppedFrames) update ≥1/sec

## Configuration Check
- Open `config/default.json` and verify:
  - `render.startupBgColor` is `"#000000FF"` (or your preferred RGBA)
  - `render.blackStartup` is `true` for fallback
- Relaunch to confirm config changes take effect immediately

## Validation Layers
- Observe launch output for Vulkan validation; verify no `VK_ERROR_*` appears
- Note any non-critical warnings for later cleanup

## Performance & Stability
- Run continuously for ≥5 minutes; confirm:
  - Stable FPS and frame time (no sustained spikes)
  - No device lost, memory errors, or crash recovery triggers
- Capture average `FrameTimeAvgMs` values and verify they remain consistent

## Logging & Artifacts
- Capture a short video or screenshots showing the black startup and running metrics
- Save console logs from a 5-minute session for baseline

## Fallback Behavior Tests
- Temporarily set `render.blackStartup=false` → relaunch → confirm behavior changes
- Restore `render.blackStartup=true` → relaunch → confirm black fallback restored

## Optional Improvements (Next Iteration)
- Add a simple on-screen performance overlay toggle (F1) rendered in-window
- Implement dynamic rendering or a minimal render pass to draw ASCII directly to swapchain (instead of console-only)
- Add CLI flag `--startup-bg #RRGGBBAA` to override config at runtime
- Add unit tests verifying config parsing for `render.*` keys

## Acceptance Criteria
- Black startup background is visible on launch across relaunches
- Metrics update ≥1/sec and average frame time stable
- No critical Vulkan errors reported during init or runtime
- Config toggles (`startupBgColor`, `blackStartup`) function as expected
- Logs and screenshots provided for confirmation