## Goal
Provide visible in-window output instead of console-only by compositing an RGBA frame to the swapchain each frame, plus an on-screen performance overlay.

## Minimal Graphics-Free Path (Transfer-Only)
- Create `FrameImage` (RGBA8) matching `swapchainExtent` in `VulkanContext`: `VkImage` + device memory
- Create host-visible `StagingBuffer` for per-frame pixel data
- Per-frame flow:
  - CPU writes ASCII-derived pixels into `StagingBuffer`
  - Clear swapchain image to `render.startupBgColor` (already implemented)
  - `vkCmdCopyBufferToImage` from `StagingBuffer` → current swapchain image (layout transfer dst)
  - Transition image to `present` and present
- Advantages: no render pass, no graphics pipeline or shaders; uses only transfer ops

## ASCII → Pixels
- Implement `Renderer::ComposeFramePixels(width,height)` that converts the current ASCII frame into RGBA pixels:
  - Simple cell-based rendering: each character maps to per-cell intensity/color
  - Optionally use existing font atlas (ASCIIConverter) later for proper glyphs; start with solid block fill per cell for speed
- Color: use palette roles; foreground per char; background from `render.startupBgColor`

## Performance Overlay
- Compose overlay text (FPS, FrameMs, RenderMs, PresentMs) into top-left region by drawing small monospace cells over the pixel buffer
- Toggle with `F1` (already used for logging): add a flag to draw overlay when enabled

## Vulkan Additions
- `VulkanContext`:
  - `CreateFrameImage(width,height)` and `CreateStagingBuffer(size)`
  - `UpdateFramePixels(const void* data,size)` maps staging buffer and writes data
  - In `BeginFrame`: acquire image and begin CMDBUF as now
  - In `EndFrame`: after clear, enqueue `vkCmdCopyBufferToImage` and present (keep existing semaphores)

## Config
- Use `render.startupBgColor` for clear color
- Add optional `render.overlayEnabled=true` default

## Validation
- Verify window shows ASCII inside the window (no white screen)
- Overlay toggles with F1; metrics update ≥1/sec
- No `VK_ERROR_*`; semaphores/waits remain consistent

## Future Enhancements
- Replace block fill with proper glyph rasterization from existing font atlas (ASCIIConverter)
- Optional shader-based blit via a minimal graphics pipeline to support filtering

## Deliverables
- Code updates in `VulkanContext`, `Renderer`, `Application`
- New staging buffer and per-frame copy path
- Overlay toggle and display
- Run + screenshot/logs confirming in-window visuals