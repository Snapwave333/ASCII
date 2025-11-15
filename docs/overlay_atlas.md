# Overlay and Atlas Glyphs

## Overlay
- Toggle: Press `F1` to show/hide the overlay in real time.
- Position: Config key `render.overlayPosition` supports `top_left`, `top_right`, `bottom_left`, `bottom_right`.
- Metrics: Displays `FPS`, `FrameMs`, `RenderMs`, `PresentMs`, `Jitter`, `DroppedFrames`.
- Adaptive: Overlay width auto-sizes to content and canvas resolution.

## Atlas Glyphs
- Generation: Uses FreeType to render ASCII glyphs at configurable `ascii.fontSize`.
- Packing: Shelf-based packing minimizes atlas height for performance.
- Metrics: Per-glyph metrics stored (UV, size, bearings, advance).
- Kerning: Kerning pairs are computed when supported by the font.
- Access: `ASCIIConverter::GetGlyphMap()` and `ASCIIConverter::GetKerning(left, right)`.

## Integration
- Backward compatible public APIs preserved.
- Renderer composes overlay via `Renderer::UpdateOverlay()` before pixel composition.
- Monitoring: Metrics are appended to `staging_run.log` per frame.

## QA
- Validate overlay toggle and positioning across window sizes.
- Verify atlas completeness for printable ASCII range and kerning where available.
- Benchmark draw and composition to maintain target frame times.

## Deployment
- Ship as a discrete update; no public API changes required.
- Monitor `staging_run.log` after deployment for performance trends.