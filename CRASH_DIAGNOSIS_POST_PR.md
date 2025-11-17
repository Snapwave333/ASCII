# Post-PR Crash Diagnosis Report

## Executive Summary

After the windowed rendering PR (`fix/windowed-rendering`), the application now **creates a window successfully but crashes during the render loop** due to a **critical out-of-bounds array access bug** in the Vulkan semaphore indexing code.

---

## ROOT CAUSE: Out-of-Bounds Semaphore Access

### Location
**File:** `src/VulkanContext.cpp`
**Lines:** 1066, 1076

### The Bug

```cpp
// Line 1066 - BUG: Uses m_currentImageIndex instead of m_currentFrame
submitInfo.pSignalSemaphores = &m_renderFinishedSemaphores[m_currentImageIndex];

// Line 1076 - BUG: Same issue
presentInfo.pWaitSemaphores = &m_renderFinishedSemaphores[m_currentImageIndex];
```

### Why It Crashes

- `m_renderFinishedSemaphores` is sized to `MAX_FRAMES_IN_FLIGHT` = **2** (`include/VulkanContext.h:117`)
- `m_currentImageIndex` is the swapchain image index, which ranges from **0 to numSwapchainImages-1**
- For triple-buffered swapchains (common on modern GPUs), this can be **0, 1, or 2**
- When `m_currentImageIndex = 2`, accessing `m_renderFinishedSemaphores[2]` is **OUT OF BOUNDS**
- This causes **undefined behavior** → crash (access violation/segfault)

### The Fix

Replace `m_currentImageIndex` with `m_currentFrame` on lines 1066 and 1076:

```cpp
// Line 1066 - FIXED
submitInfo.pSignalSemaphores = &m_renderFinishedSemaphores[m_currentFrame];

// Line 1076 - FIXED
presentInfo.pWaitSemaphores = &m_renderFinishedSemaphores[m_currentFrame];
```

**Rationale:** `m_currentFrame` cycles between 0 and `MAX_FRAMES_IN_FLIGHT-1` (line 1094), so it's always a valid index.

---

## ADDITIONAL ISSUES FOUND

### 1. Performance Monitoring Stub (Medium Priority)
**File:** `src/Application.cpp:876-892` (after PR changes)

```cpp
// Performance monitoring placeholder - methods not implemented yet
// TODO: Implement UpdatePerformanceMetrics, GetPerformanceMetrics, and frame statistics logging
```

**Impact:** No performance monitoring, but doesn't cause crashes.

### 2. AIConductor Excluded (Low Priority)
**File:** `CMakeLists.txt:64`

```cmake
list(FILTER SOURCES EXCLUDE REGEX ".*/src/AIConductor.cpp$")
```

**Impact:** AI features disabled. `NEONGLYPH_HAVE_ONNXRUNTIME=0` gates this properly, so no crash.

### 3. DirectorClientTcp.cpp Included But Director Disabled (Low Priority)
**File:** `CMakeLists.txt:65` (commented out exclusion)

```cmake
# list(FILTER SOURCES EXCLUDE REGEX ".*/src/DirectorClientTcp.cpp$")
```

**Impact:** Code compiles but `NEONGLYPH_HAVE_DIRECTOR=0` (line 154) prevents usage. No crash.

---

## COMPLETE FIX IMPLEMENTATION

### Priority 1: Fix Out-of-Bounds Access (CRITICAL)

In `src/VulkanContext.cpp`, change:

**Line 1066:**
```cpp
// FROM:
submitInfo.pSignalSemaphores = m_swapchain != VK_NULL_HANDLE ? &m_renderFinishedSemaphores[m_currentImageIndex] : nullptr;

// TO:
submitInfo.pSignalSemaphores = m_swapchain != VK_NULL_HANDLE ? &m_renderFinishedSemaphores[m_currentFrame] : nullptr;
```

**Line 1076:**
```cpp
// FROM:
presentInfo.pWaitSemaphores = &m_renderFinishedSemaphores[m_currentImageIndex];

// TO:
presentInfo.pWaitSemaphores = &m_renderFinishedSemaphores[m_currentFrame];
```

### Priority 2: Add Bounds Checking (Safety Measure)

Add validation at the beginning of `EndFrame()` (around line 1015):

```cpp
// Validate image index is within bounds
if (m_currentImageIndex >= m_swapchainImages.size()) {
    std::cerr << "[VulkanContext] Error: Current image index " << m_currentImageIndex
              << " exceeds swapchain size " << m_swapchainImages.size() << std::endl;
    return Result::InvalidArgument;
}
```

---

## CRASH SCENARIO ANALYSIS

### Timeline

1. **Startup**: Application initializes successfully
2. **Window Creation**: GLFW window created (PR fix working)
3. **Vulkan Init**: Instance, device, swapchain created
4. **First Frame**: `BeginFrame()` acquires swapchain image (index could be 0, 1, or 2)
5. **Frame End**: `EndFrame()` attempts to signal/wait on `m_renderFinishedSemaphores[2]`
6. **CRASH**: Out-of-bounds access → undefined behavior → crash

### Why Wasn't This Caught?

1. **Code Review Gap**: The semaphore indexing logic predates the PR
2. **Headless Mode**: When `NEONGLYPH_HAVE_GLFW=0`, this code path was never executed
3. **Testing Gap**: No unit tests for frame synchronization edge cases
4. **Variable Naming**: `m_currentImageIndex` and `m_currentFrame` have similar names, easy to confuse

---

## VERIFICATION STEPS

After applying the fix:

1. **Rebuild**:
   ```bash
   cmake --build build64 --config Release
   ```

2. **Test Multiple Frame Scenarios**:
   - Launch application
   - Verify window appears
   - Wait for at least 10 frames (to cycle through swapchain images)
   - Confirm no crashes

3. **Check Logs**:
   ```
   [VulkanContext] ClearColor RGBA=...
   [Window] PollEventsMs=...
   TS=... RenderMs=... PresentMs=... FrameMs=...
   ```

4. **Stress Test**:
   - Minimize/maximize window
   - Resize window
   - Let it run for 30+ seconds

---

## REGRESSION PREVENTION

### Recommended Unit Tests

1. **Test semaphore index bounds**:
   ```cpp
   TEST(VulkanContext, SemaphoreIndexNeverExceedsFramesInFlight) {
       // Simulate multiple frame cycles
       for (int i = 0; i < 100; i++) {
           context.BeginFrame();
           ASSERT_LT(context.GetCurrentFrame(), MAX_FRAMES_IN_FLIGHT);
           context.EndFrame();
       }
   }
   ```

2. **Test swapchain image acquisition**:
   ```cpp
   TEST(VulkanContext, SwapchainImageIndexValid) {
       context.BeginFrame();
       ASSERT_LT(context.GetCurrentImageIndex(), context.GetSwapchainImageCount());
   }
   ```

### Code Review Checklist

- [ ] All array indices use appropriate index variable (frame vs image)
- [ ] Bounds checking for all dynamic arrays
- [ ] Semaphore/fence operations use `m_currentFrame` not `m_currentImageIndex`
- [ ] Swapchain-related operations use `m_currentImageIndex`

---

## SUMMARY

**Root Cause**: Out-of-bounds array access in `VulkanContext::EndFrame()` using wrong index variable

**Severity**: CRITICAL (causes crash)

**Fix Complexity**: LOW (2 line changes)

**Risk**: LOW (simple index correction, well-understood Vulkan semantics)

**Files to Modify**:
- `src/VulkanContext.cpp` (lines 1066, 1076)

---

**Report Generated:** 2025-11-17
**PR Analyzed:** fix/windowed-rendering
**Commit:** 0538238
