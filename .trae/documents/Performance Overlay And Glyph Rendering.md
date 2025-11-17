## Overlay

* Draw FPS, FrameMs, RenderMs, PresentMs text into the pixel buffer’s top-left region

* Toggle with `F1`; add `render.overlayEnabled` and `render.overlayOpacity`

* Ensure overlay respects `startupBgColor` and palette foreground

## Proper Glyphs

* Use `ASCIIConverter` font atlas to render characters per cell (bitmap blit)

* Implement simple nearest-neighbor sampling; later add bilinear option

* Keep per-cell color from `m_fgColor`

## Copy Path Optimizations

* Persistently map the staging buffer and write directly without map/unmap per frame

* Add double-buffering for staging to avoid GPU-CPU sync stalls

* Optionally use `VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT` for future compute path

## Validation Cleanup

* Run with validation layers ON and resolve any non-critical warnings from the new copy path

* Confirm no `VK_ERROR_*` during init/present

## Config & CLI

* Add `overlayEnabled`, `overlayOpacity`, `overlayPosition` to `config/default.json`

* Add CLI flag `--overlay on|off` and `--startup-bg #RRGGBBAA`

## QA & Benchmarks

* Capture 5-minute logs: average `FrameTimeAvgMs`, jitter, and present time

* Verify CPU usage stays within target; confirm no dropped frames

* Save screenshots/video showing overlay and glyph rendering in-window

## Deliverables

* Code updates in `Renderer`, `ASCIIConverter`, and `VulkanContext` (overlay draw + atlas blit)

* Config additions and runtime toggles

* Logs/screenshots and a short summary of performance before/after

