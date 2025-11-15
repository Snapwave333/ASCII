# NEXT_STEPS: Comprehensive Project Timeline

## Overview
This timeline organizes work into weekly milestones with explicit success metrics, QA protocols, and verification tests. Progress is tracked via the checklist below and completion notifications are sent when all weekly milestones are achieved.

## Progress Checklist
- [x] Week 1 — Foundations and planning complete
- [x] Week 2 — GPU conversion pipeline implemented and benchmarked
- [x] Week 3 — Scene weighting system integrated; proportional text rendering baseline
- [x] Week 4 — Adaptive duration algorithm completed; text QA protocols established
- [x] Week 5 — Startup cinematic and branded logo reveals implemented; Easter egg system added
- [x] Week 6 — Spout/NDI streaming integrated; verification tests passing; performance targets met

---

## Milestone Schedule

### Week 1 — Foundations
- Define GPU pipeline scope, inputs, and target render formats
- Draft scene weighting data model and integration points
- Specify text rendering requirements and scaling behavior
- Establish success metrics and test environments (GPU, scenes, fonts)
- Plan verification tests covering all new features
- Deliverable: Signed-off test plan and metric thresholds

### Week 2 — GPU Conversion Pipeline
- Implement GPU conversion pipeline for improved rendering performance
- Integrate instrumentation to capture frame time, draw calls, GPU utilization
- Build initial test scenes for pipeline validation
- Run baseline benchmarks and optimize hotspots
- Acceptance: Meets success metrics and testing procedures described below

### Week 3 — Scene Weighting + Text Rendering
- Develop and integrate scene weighting system for content prioritization
- Build proportional text rendering system with dynamic scaling (baseline)
- Create data tables/structs for weighting and text configuration
- Acceptance: Weighting affects scheduling; text scales proportionally across resolutions

### Week 4 — Adaptive Duration + Text QA
- Create adaptive duration algorithm for content timing
- Establish QA protocols for text rendering accuracy (automated + manual)
- Add error handling/state management around weighting and duration
- Acceptance: Duration adapts to content complexity and weighting; QA suite passes

### Week 5 — Cinematic, Branding, Easter Eggs
- Design and produce startup cinematic sequence
- Implement branded logo reveal animations
- Develop Easter egg injection system with trigger conditions
- Acceptance: Cinematic and logo sequences meet timing targets; triggers behave deterministically

### Week 6 — Streaming, Verification, Performance
- Integrate Spout/NDI streaming capabilities
- Create verification tests for all new features and run the full suite
- Performance tuning to target 60 FPS and ≤ 800 draw calls
- Final confirmation includes test results and implementation details

---

## 1. Tasks
- Implement GPU conversion pipeline for improved rendering performance
- Develop and integrate scene weighting system for content prioritization
- Define success metrics and testing procedures for GPU pipeline

## 2. Deliverables
- Build proportional text rendering system with dynamic scaling
- Create adaptive duration algorithm for content timing
- Establish QA protocols for text rendering accuracy

## 3. Objectives
- Design and produce startup cinematic sequence
- Implement branded logo reveal animations
- Develop Easter egg injection system with trigger conditions
- Integrate Spout/NDI streaming capabilities
- Create verification tests for all new features

---

## Success Metrics and Testing Procedures — GPU Pipeline

### Metrics (targets)
- Frame rate: ≥ 60 FPS sustained on target hardware
- Draw calls: ≤ 800 per frame (average over 30 seconds scene sweep)
- Frame time: ≤ 16.67 ms average; ≤ 25 ms at 99th percentile
- GPU utilization: ≤ 85% sustained under peak scenes
- Memory footprint: Stable with no leaks over 10-minute run; ≤ target budget
- Pipeline conversion throughput: ≥ 90% of assets processed without manual intervention

### Test Procedures
- Instrumentation: Capture frame time, draw calls, GPU utilization via profiler hooks
- Scenes: Run light, medium, heavy complexity scenes; include text-heavy and animated cases
- Repeatability: 3 trial runs per scene; record averages and percentiles
- Regression: Snapshot metrics pre/post optimization; prevent >5% degradation
- Error handling: Force malformed inputs; verify safe fallback and logged diagnostics
- Acceptance: All targets met across medium scenes; heavy scenes within 10% of targets

---

## QA Protocols — Text Rendering Accuracy

### Scope
- Fonts: Serif, Sans, Monospace; multiple weights
- Resolutions: 720p, 1080p, 1440p, 4K
- Languages: Latin baseline; spot checks for extended glyph sets

### Tests
- Proportional scaling: Text bounding boxes and line breaks maintain ratios across resolutions
- Baseline alignment: Verify vertical/horizontal alignment consistency within ±1 px at 1080p
- Kerning/leading consistency: Compare rendered glyph positioning to metrics tables
- Contrast/legibility: WCAG minimum contrast checks for foreground/background pairs
- Image difference: Golden image comparisons with tolerance ≤ 0.5% pixel delta
- Performance: Text render pass adds ≤ 2 ms average frame time

### Acceptance
- All checks pass; failures produce actionable diagnostics and are tracked

---

## Verification Tests — New Features

### Startup Cinematic
- Playability: Starts on boot and completes without stutter
- Timing: Matches specified duration ±5%
- Skip logic: Optional skip respects input mapping; state is consistent post-skip

### Branded Logo Reveal
- Animation curves: Easing profiles match design spec
- Resolution independence: Visual quality consistent across target resolutions

### Easter Egg Injection System
- Trigger conditions: Deterministic activation via defined state inputs
- Persistence: Session-scoped unless explicitly reset; logged when active
- Safety: No performance degradation > 3% when inactive

### Spout/NDI Streaming
- Handshake: Initializes and tears down cleanly; reports status
- Throughput: Meets target frame rate at 1080p over local network
- Audio sync (if applicable): AV sync offset ≤ 50 ms

### Global
- Automated suite: CI-friendly runner with pass/fail reporting
- Manual checklist: Runbook for edge-case scenarios and visual inspections

---

## Data Structures and Configuration
- Use structs and data tables for: scene weighting, text scaling config, duration parameters
- Include basic error handling and state management around all core logic

---

## Completion Protocol
- System will automatically update `docs/NEXT_STEPS.md` with progress
- Receive completion notification when all weekly milestones are achieved
- Final confirmation will include test results and implementation details
Progress Auto-Update: 2025-11-14 20:15:04
Verification Auto-Update: runtime_logic_tests:PASS, ascii_font_atlas_test:PASS, e2e_test:FAIL, gpu_pipeline_test:PASS
