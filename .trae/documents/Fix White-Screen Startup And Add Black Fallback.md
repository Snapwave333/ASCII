## Problem Overview
- Startup shows a blank white window because there is no swapchain render/present path; the window gets the platform default background.
- Renderer and theme code default many fallbacks to white, compounding a white appearance.
- Validation logs show semaphore misuse during `EndFrame`, which can stall or produce undefined visual state.

## Root-Cause Evidence
- No Vulkan clear/present code exists (no `VkClearValue`, `vkCmdBeginRenderPass`, `vkCmdBeginRendering`, `vkAcquireNextImageKHR`, `vkQueuePresentKHR`).
- Renderer defaults to white: `src/Renderer.cpp:216–226` (`ResizeCanvas`, `ClearCanvas` set `m_fgColor=0xFFFFFFFFu`), `ComposeFrameString` resets on white `src/Renderer.cpp:252–260`.
- Theme color parsing returns white on error: `src/Theme.cpp:80–95`.
- Palette and config parsing fallbacks to white are widespread.
- Platform integration does not paint the window background.

## Implementation Plan
### 1) Immediate Visual QoL: Black Fallback Defaults
- Change default color fallbacks to black where “background” is implied:
  - `ThemeManager::HexToRGBA` error fallback → `0x000000FF` (Theme.cpp:80–95).
  - `Renderer` startup canvas foreground used for blank frames → initialize to black rather than white: `ResizeCanvas` & `ClearCanvas` (Renderer.cpp:216–226).
- Add config key `render.startup_bg_color` with default `#000000` and load it in `ConfigManager` to drive renderer’s initial canvas color.
- Ensure palettes/themes respect `palette.background` and prefer black for “music venus” profile.

### 2) Frame-Readiness Gate
- Introduce `first_frame_ready` in `Application` and `Renderer`:
  - Until ASCII/compute output is ready, present a black frame (no content) instead of a white canvas.
  - Log readiness transitions and durations.

### 3) Minimal Vulkan Clear-To-Black (No Graphics Pipeline)
- Implement swapchain management in `VulkanContext` (CreateSwapchain, ImageViews, Choose formats/modes, Acquire/Present).
- Implement per-frame sequence:
  - Acquire next image (`vkAcquireNextImageKHR`).
  - Record command buffer:
    - Transition swapchain image to `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL`.
    - `vkCmdClearColorImage` with `{0,0,0,1}` to clear to black.
    - Transition to `VK_IMAGE_LAYOUT_PRESENT_SRC_KHR`.
  - Submit and present (`vkQueuePresentKHR`).
- Use this path until `first_frame_ready=true`. When ready, keep the clear step and overlay compute result if/when you add graphics blit later.

### 4) Fix Validation-Layer Warnings
- Review `VulkanContext::EndFrame` submission:
  - Only include semaphores that are signaled within that submission.
  - Ensure binary semaphores are unsignaled before use in `pSignalSemaphores` and that waits match signal sources.
  - If semaphores are not needed, remove them to avoid VUID `pCommandBuffers-00070`/`pSignalSemaphores-00067` spam.

### 5) Logging & Telemetry
- Add explicit logs: window background chosen, swapchain format, present mode, image transitions, and first frame timing.
- Add counters for startup black-fallback frames and time-to-first-frame.

### 6) Verification
- Run with validation layers to confirm no critical warnings.
- Visual check: black screen at startup, transition to ASCII visuals once ready.
- Measure startup latency and confirm no dropped frames during initial period.
- Add a config toggle to force black startup for “music venus” sessions.

## Deliverables
- Code updates to `Renderer.cpp`, `Theme.cpp`, `ConfigManager.cpp`, `VulkanContext.{h,cpp}` and minimal swapchain path.
- New config keys with defaults (`render.startup_bg_color`, optional `render.black_startup=true`).
- Startup readiness gating with logs.
- Validation fixed submissions.

## Rollout
- Implement and test on Windows with NVIDIA/Intel hybrid.
- Keep validation layers enabled during development; disable in production after verification.
- Provide a short video/GIF capture to confirm black fallback and smooth transition.

Proceed with this plan?