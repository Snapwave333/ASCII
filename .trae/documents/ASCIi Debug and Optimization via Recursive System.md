## Objectives
- Stabilize and optimize the ASCIi project to maintain 60 FPS with consistent latency, reduce CPU/GPU bottlenecks, and improve reliability without regressions.
- Apply the recursive optimization architecture (orchestrator, evaluator, meta-optimizer, memory ledger) to continuously refine code paths and configurations.

## Baseline & Instrumentation
- Add high-resolution timers and counters around key hot paths:
  - ASCII conversion CPU `ASCIIConverter::ConvertSIMD` in `src/ASCIIConverter.cpp:617` and GPU `ConvertGPU` in `src/ASCIIConverter.cpp:681`.
  - Frame composition `Renderer::ComposeFramePixels` in `src/Renderer.cpp:227` and `Renderer::ComposeFrameString` in `src/Renderer.cpp:287`.
  - Vulkan frame lifecycle `VulkanContext::BeginFrame` in `src/VulkanContext.cpp:754` and `VulkanContext::EndFrame` in `src/VulkanContext.cpp:889`.
  - Output pipeline in `src/OutputManager.cpp` (NDI/Spout send path) and audio capture in `src/AudioEngine.cpp`.
- Collect per-frame metrics: CPU wall time, GPU time (timestamps), memory allocations, queue depths, dropped frames, audio latency, cache hit/miss.
- Persist metrics via `Logger` with structured records; enable optional CSV/JSON for offline analysis.

## Composite Metrics & Feedback Loops
- Composite quality score `Q` per build: FPS stability, median/95p frame time, CPU load, GPU load, memory churn, dropped frames, audio latency, Vulkan stalls, output throughput, constraint adherence (draw calls <= 800).
- Iterative loop: generate N optimization candidates, benchmark, score, select top-k, apply focused edits, re-score, stop on convergence or budget.
- Strategy evolution: bandit-driven selection of tactics (SIMD tuning, GPU dispatch, caching changes, threading parameters); Bayesian tuning of continuous knobs (block sizes, queue capacities, TTLs).

## Optimization Candidates (First Cycles)
- ASCII conversion CPU (`src/ASCIIConverter.cpp:617`):
  - Ensure 32-byte alignment, prefetch, and contiguous memory traversal; reduce branches in luminance mapping; use LUTs for ANSI color mapping.
  - Evaluate tile-based AVX processing to improve cache locality; minimize gathers; batch contrast/brightness operations.
- ASCII conversion GPU (`src/ASCIIConverter.cpp:681`):
  - Use persistent descriptor sets and command buffers; avoid per-frame recreation; optimize barriers; tune workgroup size (e.g., 16x16 vs 8x8) based on device.
  - Measure readback costs; prefer GPU-side string/materialization where possible.
- Frame composition (`src/Renderer.cpp:227`, `src/Renderer.cpp:287`):
  - Replace per-pixel branching with vectorized operations; minimize escape code changes via run-length grouping; reserve output buffer to avoid reallocations.
  - Optimize resampling to avoid redundant computations; precompute mapping tables.
- Vulkan frame pipeline (`src/VulkanContext.cpp:754`, `src/VulkanContext.cpp:889`):
  - Reuse command buffers; double-buffer fences/semaphores; reduce layout transitions; enable timeline semaphores if supported.
  - Add GPU timestamp queries to locate stalls; adjust frames-in-flight.
- Output path (`src/OutputManager.cpp`):
  - Minimize copies: zero-copy from Vulkan image to Spout shared texture; batch NDI frames; tune encoder settings where applicable.
- Concurrency & queues (`include/concurrency/LockFreeRingBuffer.h:28`, `:45`):
  - Validate capacity sizing and false sharing; ensure avoidable busy-wait; profile enqueue/dequeue contention under load.
- Cache (`include/cache/SegmentedLRUCache.h:23` `set/get/invalidate at :32, :52, :66`):
  - Integrate for font atlas and per-char density results; tune shards/TTL; monitor hit rate and lock contention; adapt segments via meta-optimizer.

## Bug Triage & Stability
- Investigate runtime warnings/errors in logs; add asserts and safe fallbacks where needed.
- Address `TODO` in `src/Application.cpp:1125` (directive application path) to reduce logic inconsistencies that may affect performance scheduling.
- Validate headless/window fallback robustness and Director TCP thread interactions to avoid intermittent stalls.

## Meta-Learning Implementation
- Strategy graph: tactics for SIMD/GPU/caching/threading/materialization; constraints encoded (API correctness, safety).
- Bandit controller: select tactics per task features (resolution, GPU present, headless vs windowed).
- Bayesian optimizer: continuous parameters (workgroup size, buffer sizes, TTLs, frames-in-flight).
- Memory ledger: persist iteration artifacts (metrics, diffs, configs) for transfer learning across scenes/configs.

## Testing & Validation
- Run existing suites: `tests/ascii_font_atlas_test.cpp`, `tests/gpu_pipeline_test.cpp`, `tests/vulkan_resource_stress_test.cpp`, `tests/thread_lifecycle_tests.cpp`, `tests/e2e_test.cpp`.
- Add microbenchmarks: ASCII CPU/GPU conversion throughput vs resolution; frame composition throughput; queue contention under varying capacities.
- A/B and canary: compare baseline vs candidate; rollback on regressions; enforce gates for publish.
- Stress: adversarial resolutions, high ASCII color churn, rapid scene changes; monitor stability and recovery.

## Operating Cycle
- initialize(task): extract features (resolution, mode), load priors, set budgets.
- generate_candidates(policy): implement code/config variants for targeted subsystems.
- evaluate(candidates): benchmark with composite `Q` and reliability.
- refine(top_k): apply focused edits or parameter changes; re-measure.
- meta_update(history): update tactic weights and continuous knobs.
- decide(): publish if `Q >= threshold` and constraints pass; else iterate or escalate.
- persist(): store artifacts and introspection for future cycles.

## Deliverables & Targets
- Sustained 60 FPS at target resolution with 95p frame time <= 16.6 ms.
- CPU/GPU utilization balanced; reduced per-frame allocations and synchronization stalls.
- Vulkan pipeline stabilized with timestamped evidence of stall removal.
- ASCII conversion throughput improvement (≥20% CPU path, ≥30% GPU path where available).
- Cache hit rate ≥80% for targeted lookups with minimal contention.

## Next Steps Upon Approval
- Implement instrumentation and benchmarking harness.
- Execute first optimization cycles for ASCII conversion and frame composition.
- Report results with diffs, metrics, and accepted changes; proceed to Vulkan and output path cycles.
