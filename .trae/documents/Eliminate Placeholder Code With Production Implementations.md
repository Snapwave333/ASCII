## Scope Inventory
- Replace placeholder/stub logic identified across C++ and Python, focusing on runtime paths. Key locations:
  - `src/AIConductor.cpp:70–76, 150–193, 239, 277, 476–489, 680–682`
  - `src/ComputePipelines.cpp:200–206, 297–304, 385–396`
  - `src/VulkanContext.cpp:613–616`
  - `src/OutputManager.cpp:197–245`
  - `src/IconGenerator.cpp:9–15, 64, 92` and `include/IconGenerator.h:1, 47`
  - `src/SafetyManager.cpp:292, 319`
  - `src/Theme.cpp:283` and `include/NeonGlyph.h:90`
  - App-level TODOs: `src/Application.cpp:463, 802, 818`
  - Python stubs: `tests/e2e/test_app_playwright.py:19,49`, `tools/update_next_steps.py:79,91`, plus other `pass` usages in LLM/seamless integration files.

## Prioritization
- High impact: `AIConductor`, `ComputePipelines` (FFT/BPM/SPIR-V), `VulkanContext` compute init, `OutputManager` interop.
- Medium: `IconGenerator`, `SafetyManager`, app-level TODOs.
- Low: Test/utility Python stubs unless they gate CI.

## Implementation Strategy
### AIConductor
- Replace rule-based genre/mood with deterministic DSP:
  - Implement on-CPU feature extraction (spectral centroid/rolloff/flux, RMS, zero-crossing) with windowing and overlap.
  - Genre classification via trained thresholds + k-NN over a small curated feature set (no placeholders), persisted in `assets` with real data tables; keep API unchanged.
  - Mood prediction via arousal/valence mapping from features; add hysteresis and debouncing.
- Optional ONNX Runtime path guarded by `NEONGLYPH_HAVE_ONNXRUNTIME` loads real models when present; otherwise DSP path.
- Input validation: bounds checks on `frame`/`spectrum`, thread-safe queues, overflow guards, NaN handling.
- Error handling: return `Result` codes, no silent failures; log via `Logger`.

### ComputePipelines
- Audio analysis shader: replace placeholder magnitude path with proper FFT pipeline:
  - Move FFT to CPU using an in-repo radix-2 FFT (no external deps) for reliability and performance; upload spectrum to GPU buffers.
  - Alternatively provide compute-shader FFT only for power-of-two windows with Cooley–Tukey passes, validated against CPU FFT.
- Beat detection shader: implement BPM estimation using onset envelope + autocorrelation:
  - Maintain circular buffer of onset strengths; compute peak intervals and robust tempo via median/weighted histogram.
  - Replace `output.bpm` placeholder with calculated BPM; add confidence metric.
- SPIR-V compilation: use existing `glslangValidator` CLI integration (`src/ComputePipelines.cpp:450–496`); remove “placeholder” comment and ensure robust error propagation.

### VulkanContext
- Implement compute pipeline creation instead of returning success:
  - Instantiate `ComputeShaderManager`, compile built-ins (`ascii_convert`, `beat_detection`, `audio_analysis`), create descriptor sets/layouts and pipeline layout.
  - Validate device/queue availability and descriptor pool before pipeline creation.

### OutputManager (Spout/Interop)
- Replace test-pattern path with real Vulkan→D3D11 copy:
  - Implement staging buffer readback (if no interop extensions), then `Map` shared texture and copy rows respecting pitch.
  - Prefer Win32 external memory interop: `VK_KHR_external_memory_win32` + `ID3D11Device1::OpenSharedResource1` for zero-copy; feature-detect and fallback to staging path.
- Validate width/height changes, recreate resources safely, propagate errors.

### IconGenerator
- Implement real ICO generator:
  - Build multi-size images (16/32/48/64/128) with proper BITMAPINFOHEADER/PNG entries; write valid ICO header and directory entries.
  - Generate pixel data from real app glyph density tables instead of placeholders.

### SafetyManager
- Implement real file logging with rotation, timestamps, and sanitization; ensure thread-safety and error handling.

### Theme/Result NotImplemented
- Replace `Result::NotImplemented` returns with working paths or feature-gated returns where functionality is unsupported; maintain backward-compatible enums.

### Application TODOs
- Fullscreen toggle: hook into windowing system (`Window.cpp`) with validation.
- Palette/charset application: wire to `ASCIIConverter` config updates with bounds checks.

## Error Handling & Validation
- Use `Result` consistently; avoid exceptions on hot paths.
- Validate inputs (null handles, size/stride, power-of-two FFT windows, descriptor counts).
- Add timeouts, thread priority bounds, and queue limits to avoid starvation.

## Performance & Security
- Performance: reuse buffers, avoid redundant allocations, vectorized CPU FFT, minimize GPU stalls, use pipeline caches.
- Security: sanitize external tool paths, avoid command injection in `glslangValidator` spawn, validate file IO, keep content filtering robust.

## Testing Plan
- Unit tests: CPU FFT correctness (impulse/sine/sum), BPM estimation vs synthetic click tracks, icon writer verifies ICO headers.
- Integration tests: GPU pipeline convert (`tests/gpu_pipeline_test.cpp`), compute dispatch, Vulkan present.
- E2E: Output stream validation (frame rate, resolution), AIConductor end-to-end feature-to-decision.
- Python tests: replace `pass` with real assertions where applicable.

## Backward Compatibility
- Preserve public APIs, `Result` semantics, and feature flags; optional ONNX path gated.

## Code Review & Documentation
- Perform internal review on each module update; document public APIs in headers only if allowed; update existing MD guides minimally.

## Milestones
1. Implement CPU FFT + tests; wire spectrum upload.
2. Replace BPM placeholder with autocorrelation; validate on test signals.
3. Complete VulkanContext compute pipeline init.
4. Implement Spout interop copy paths; validate FPS and latency.
5. AIConductor DSP classifier and mood mapping; verify with tests.
6. IconGenerator real ICO writer; tests for header correctness.
7. SafetyManager logging; app-level TODOs.
8. Replace remaining stubs in Python tests/utilities.

Confirm to proceed; after approval I will implement and verify all items iteratively with unit/integration tests, keeping backward compatibility and performance targets.