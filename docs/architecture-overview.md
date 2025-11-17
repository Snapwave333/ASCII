## Swapchain Format Selection

Format strategy: prefer `VK_FORMAT_B8G8R8A8_UNORM` with `VK_COLOR_SPACE_SRGB_NONLINEAR_KHR`. Fallback to the first available format if preference is unavailable.

Rationale:
- `B8G8R8A8_UNORM` is widely supported on Windows GPUs and aligns with typical presentation paths.
- Avoids per-frame conversion: CPU-composed pixels match GPU format directly.

Implementation: `VulkanContext::ChooseSwapSurfaceFormat` selects BGRA (`src/VulkanContext.cpp:1379–1382`). Swapchain creation applies the chosen format (`src/VulkanContext.cpp:1265–1299`).

Critical dependency: `Renderer::ComposeFramePixels` must pack bytes as BGRA to match the swapchain format (`src/Renderer.cpp:254–257`). Any change to swapchain format requires a corresponding change to pixel packing.