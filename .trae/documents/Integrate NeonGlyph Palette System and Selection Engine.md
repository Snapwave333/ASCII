## Overview
Implement a robust, production-grade color system based on the provided JSON: new rich palette schema, phase-aware randomization rules, and a mood/tempo-driven selection engine. Integrate at three layers: config loading, selection in the directing layer, and rendering application (including improvisation/morphing behavior with guardrails).

## Files to Touch
1. Config loading: `src/ConfigManager.cpp`, `include/ConfigManager.h`
2. Palette data & manager: `include/NeonGlyph.h` (extend types), `src/PaletteManager.cpp` (new), `include/PaletteManager.h` (new)
3. Selection engine: `src/PaletteManager.cpp`
4. Director integration: `src/AIDirector.cpp`, `include/AIDirector.h`
5. Story/phase inputs: `src/StoryContext.cpp`, `include/StoryContext.h` (non-breaking additions)
6. Renderer & ASCII color application: `src/Renderer.cpp`, `include/Renderer.h`, `src/ASCIIConverter.cpp`, `include/ASCIIConverter.h`
7. Optional prompt add-on (color improvisation block): `src/ScenarioManager.cpp`

## Data Structures
- `struct RichPalette` (extends current `ColorPalette`):
  - `name`, `role_tags`, `mood_tags`, `scene_tags`, `temperature`, `energy`, `brightness`
  - `primary`, `secondary`, `secondary2`, `accent`, `shadow`, `highlight` (all `uint32` RGBA)
- `struct PhaseRules` with `preferred_role_tags`, `avoid_role_tags`, `avoid_moods`, `fallback_role_tags`, `bias_moods`, `bias_energy`
- `struct SelectionInput`: `phase`, `mood`, `tempo_bpm`, `energy_level_0_1`, `color_mode`, `is_dark_venue`, `is_test_sequence`
- `class PaletteManager`:
  - Holds `std::vector<RichPalette>`, `RandomizationRules`, per-phase recent history (size from `non_repetition.max_recent_history`)
  - API: `LoadFromJson(path)`, `SelectPalette(const SelectionInput&)`, `GetCurrent()`, `PushHistory(phase, name)`

## Config Loading
- Use vendored `nlohmann::json` (already present in build system) to parse the provided JSON into `RichPalette` and `RandomizationRules`.
- Backward-compatible: keep existing `ParsePaletteConfig` for legacy `{"palettes": {name: {colors: [...]}}}` files; auto-detect schema by top-level keys.
- Expected file location: `config/palettes.json` (drop the provided JSON as-is).

## Selection Engine
- Filtering:
  - Phase map resolves via `phase_rules[phase]` → filter by `preferred_role_tags`; if empty, use `fallback_role_tags`.
  - Exclude palettes with any `avoid_role_tags` or `avoid_moods` for phase.
- Scoring components (weights tuned conservatively, adjustable via config):
  - Mood match: +2 per matching `mood_tags` across input `mood` list
  - Temperature match: +1 if equals requested mood temperature (derived from input and venue darkness)
  - Energy alignment: +0–2 based on proximity of `energy` to discretized `energy_level_0_1` (map 0–1 → low/medium/high/extreme)
  - Brightness fit: +1 if `brightness` suits `is_dark_venue` and `color_mode` (prefer `high` for dark venue truecolor; prefer `medium`/`low` for bright venue/ANSI)
  - Role synergy: +1 if palette role tags include phase’s preferred roles
  - Tempo synergy: +0–2 where BPM bands (≤90, 90–120, 120–150, >150) prefer low→extreme energy respectively
- Non-repetition:
  - Maintain per-phase ring buffer of last `N` selections; exclude unless no valid options remain.
- Tie-breaking:
  - Pick highest score; if tie, sample by weight with slight random jitter; fallback to least-recently-used.
- Output:
  - Return `RichPalette` and a `ColorStrategy` suggestion: `static`, `pulsing`, `gradient`, `morph`, `hybrid` based on mood/tempo/phase.

## Director Integration
- `AIDirector::Update` (c:\Users\chrom\Documents\trae_projects\ASCIi\src\AIDirector.cpp):
  - Gather `StoryContext` (phase) and `MusicData` (tempo/energy) → build `SelectionInput`.
  - Call `PaletteManager::SelectPalette` → set `DirectorState.palette` and `DirectorState.color_strategy`.
  - Emit a `DirectorCommand` with `SetPalette(name)` and optional `SetColorStrategy(strategy)` for `Renderer`.

## Renderer & ASCII Application
- Implement truecolor path using `RichPalette` (`primary/secondary/...`) and region-aware application in `Renderer::ComposeFrameString`.
- ANSI mode keeps `nearestAnsi` mapping but biases towards palette hues.
- `ASCIIConverter` (TODO hook) receives current palette to influence quantization and luminance thresholds.

## Improvisation & Morphing Layer
- Append the provided “COLOR IMPROVISATION & MORPHING FREEDOM” block to the agent’s preamble when generating color-related scenarios/prompts (`ScenarioManager::Generate`).
- Implement `ColorMorpher` utilities:
  - Hue drift, saturation pulse, luminance breathing synchronized to audio bands (via `AIConductor` hooks)
  - Local micro-palettes per region (headlights, flames, beams, stars) harmonized to base palette
- Guardrails enforced:
  - Contrast checks (foreground/background luminance delta), strobe comfort limits, coherence threshold (limit simultaneous palette layers).

## Story/Phase Mapping
- Map engine phases to existing flow:
  - `startup_intro` → initial `intro` section in `Application::MainLoop`
  - `logo_reveal` → `RenderNeonGlyphLogo` directive path
  - `test_card` → `MorphToTestCard`/`RenderTestCard`
  - `live_performance` → `LivePerformanceSystem::LIVE`

## Verification
- Logging: on selection, log phase, input features, chosen palette name, and score breakdown.
- Determinism option for test sequences: set fixed RNG seed when `is_test_sequence=true`.
- Performance: zero-copy palette access; no new draw calls; maintains current FPS budget.
- Manual scenario demo: scripted sequence exercising all phases with varied mood/tempo to validate non-repetition.

## Deliverables
- `PaletteManager` with schema parsing, rules, and selection.
- Director and renderer integration emitting visible color changes in both truecolor and ANSI modes.
- Optional prompt add-on correctly appended without modifying core prompt or schema.

## Rollout Notes
- No external runtime deps; `nlohmann::json` is already vendored.
- Backward-compatible with existing simple palette configs.
- All improvisation features respect readability/safety guardrails.
