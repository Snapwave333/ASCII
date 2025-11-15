# NeonGlyph

**Document Type**: Project Overview
**Version**: 3.0.1
**Last Updated**: 2025-11-14
**Status**: Approved
**Author**: NeonGlyph Development Team

## Overview

NeonGlyph is the ultimate AI-driven ASCII visual synthesis engine capable of real-time 8K performance with AI inference, targeting VJs, streamers, and live coders who demand the "hacker" aesthetic. **Now featuring seamless E2E scaling system with complete borderless implementation, universal monitor support, and cyberpunk visual generation with flowing @ symbol patterns, neon gradients, and 60 FPS performance.**

## Mission Statement

Construct the ultimate AI-driven ASCII visual synthesis engine with the philosophy: "High Tech, Low Life, High Frame Rate"

## System Architecture

### Core Technology Stack

| Component | Technology | Rationale |
|-----------|------------|-----------|
| Core Language | C++20 | Zero-overhead abstraction for sub-16ms frame times |
| Graphics API | Vulkan | Compute shaders mandatory for 8M pixel ASCII mapping |
| AI Runtime | ONNX Runtime (DirectML) | Hardware-agnostic LLM/classification acceleration |
| Audio Engine | WASAPI Loopback | Low-level, lock-free audio capture |
| Interoperability | Spout 2 / NDI 5 | Zero-latency texture sharing to Resolume/OBS |
| Seamless Scaling | WS_POPUP / Windows API | Complete borderless implementation with zero OS chrome |
| Visual Generation | Mathematical Wave Functions | Flowing @ patterns with neon gradients and audio reactivity |

### Data Flow Pipeline

```
Audio Input → FFT Analysis → AI Director → GPU Compute → ASCII Output
    ↓              ↓             ↓            ↓           ↓
WASAPI Loopback → Spectrum → SLM Context → Density Map → Spout/NDI
```

## Development Roadmap

### Phase 1 — Core Engine Fortification (Foundations)
- 1. Autonomous Show Director Kernel — AI decides scene flow, pacing, theme, transitions.
- 2. Palette Intelligence Core — Chooses, morphs, fades, rewrites palettes dynamically.
- 3. Scene Archetype Selector — Auto-selects scene type (tunnel, fractal, portal, idol, grid, void, city, data-crawl).
- 4. Motion Grammar Engine — Implements and mixes motion modes: `tunnel_zoom`, `orbital_pan`, `panel_chase`, `kaleido_fold`, `breathing_totem`.
- 5. Layered Rendering Stack — Full depth layering with separate motion and color logic per layer (bg/mid/fg).

### Phase 2 — Live Performance Brain (Autonomous Creative Logic)
- 6. Audio Reactivity Matrix — Independent color, motion, density, and brightness mapping to bass/mids/highs.
- 7. Scene Probability Weights — Agent chooses scenes randomly but weighted by energy, mood, tempo, and previous selections.
- 8. Progressive Narrative Loops — Agent builds micro-stories: awakenings, rifts, storms, ascensions.
- 9. Adaptive Scene Duration — Scene length decided by energy and pattern saturation, not timers.
- 10. Self-Generated Cuts & Edits — Agent inserts cuts, fades, pulses, wipes at its own discretion.

### Phase 3 — Cinematic Systems (Showmanship & Identity)
- 11. Startup Hero Cinematic System — Autonomous multi-beat intro, unique each launch.
- 12. NeonGlyph Logo Reveal Engine — Multiple reveal mechanics (explosion, bloom, glitch, aperture, collapse).
- 13. Morph-to-Test-Card Ritual — Logo transforms into calibration card using dissolve/fold/rift techniques.
- 14. Test-Card Intelligence Suite — Smart brightness bars, color ramps, geometry grids, motion tests, audio meters.
- 15. Easter Egg Micro-Injector — Low-probability flashes: portal peeks, luminance inversions, hidden glyphs, data crawls.

### Phase 4 — Immersive Light + Color Realism (Advanced Visual Texture)
- 16. Real-World Color Behaviors — Cars headlights, flames, neon signs, beams, specular halos, depth fog.
- 17. Procedural Gradient Sculpting — Thick gradients with simulated volume, curvature, heat shimmer.
- 18. Multi-Zone Local Palettes — Each scene region gets its own sub-palette (sky, floor, core, halo, shards).
- 19. Color Morph Fields — Scene-wide color fields that drift, pulse, ripple, or tear.
- 20. Material Simulation Layer — ASCII textures mimic metal, glass, crystal, smoke, plasma, stone.

### Phase 5 — Meta-Autonomy (Self-Directed Evolution & Set Design)
- 21. Energy-Level Show Progression — Entire set evolves from ambient → rising → peak → finale without prompts.
- 22. Self-Adaptive Mood Engine — AI shifts mood categories (psychedelic, void, ceremonial, aurora) based on long-term energy curve.
- 23. Style-Fusion Generator — Agent creates hybrid visuals by blending palette families and motion archetypes.
- 24. Setlist Memory — Agent logs recent scenes to avoid repetition and build continuity.
- 25. Finale Intelligence — Detects session wind-down and triggers a self-composed finale (collapse, supernova, ascension).

## Safety & Quality Standards

### Epilepsy Protection Standard
- Mandatory "Photosensitive Mode" enabled by default
- Rolling average luminance delta clamping
- Legal disclaimer on app launch

### Venue Stability Standard
- Crash recovery in <2 seconds
- Watchdog process for renderer hangs
- Last-known configuration restoration

## Performance Targets

- **Resolution**: 8K (7680×4320) @ 60fps
- **Draw Calls**: <800 per frame
- **CPU Usage**: <5% on idle
- **Memory**: <2GB baseline
- **Latency**: <16ms audio-to-visual

## Target Markets

- **VJs**: Techno/Cyberpunk aesthetic seekers
- **Streamers**: Unique overlay creators
- **Live Coders**: ASCII purity enthusiasts

## Future Horizons

- VR support for 3D ASCII environments
- DMX integration for physical lighting
- Multiplayer VJ collaboration
- Advanced psychedelic visual synthesis with machine learning
- Real-time consciousness state mapping

## Quick Start

### Prerequisites
- Visual Studio 2022 with C++ development tools
- Vulkan SDK (latest version)
- CMake 3.20 or higher
- Windows 10 or later

### Build Instructions
1. Clone the repository
2. Open Visual Studio 2022 Developer Command Prompt
3. Run `build.bat` in the project root
4. Launch `build64\Release\NeonGlyph.exe`

For detailed build instructions, see [Build Guide](.trae/documents/build-guide.md)

## Documentation

- [Documentation Index](.trae/documents/DOCUMENTATION_INDEX.md) - Central navigation and cross-references
- [Documentation Style Guide](.trae/documents/DOCUMENTATION_STYLE_GUIDE.md) - Standards for structure, tone, and terminology
- [Build Guide](.trae/documents/build-guide.md) - Comprehensive build instructions
- [Architecture Overview](.trae/documents/architecture-overview.md) - System architecture details
- [API Reference](.trae/documents/api-reference.md) - API documentation
- [Configuration Guide](.trae/documents/configuration-guide.md) - Configuration options
- [Timeline & Milestones](docs/NEXT_STEPS.md) - Weekly milestones and success metrics
- [Process Completion Report](docs/PROCESS_REPORT.md) - Verification evidence and audit

## What’s New (3.0.1)

- GPU conversion pipeline with runtime SPIR-V compilation
- Scene weighting and adaptive TTL for content prioritization and timing
- Spout/NDI CPU frame streaming integrated via OutputManager
- Color parsing and BGRA packing fixes to eliminate startup blue tint
- AI orchestrator threads prioritized for faster startup
- Playwright e2e test added; verification auto-updates on build

## Contributing

Please read our [Contributing Guide](CONTRIBUTING.md) for details on our code of conduct and development process.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Support

For support, please open an issue on our GitHub repository or contact the development team.

## Change History

### Version 3.0.0 (2025-11-14)
- **MAJOR**: Added Seamless E2E Scaling System with complete borderless implementation
- **MAJOR**: Zero OS Chrome using WS_POPUP style for perfect visual purity
- **MAJOR**: Universal Monitor Support with automatic detection and adaptation to any display configuration
- **MAJOR**: Perfect Pixel Coverage with exact screen dimension coverage at 0,0 positioning
- **MAJOR**: 60 FPS Performance with smooth ASCII animation and mathematical character density calculation
- **MAJOR**: Cyberpunk Visual System with flowing @ symbol patterns, neon gradients, and audio reactivity
- **MAJOR**: Complete Control Integration with ESC, Double-click, Alt+B/S/P shortcuts for seamless control
- Enhanced mathematical character density calculation for optimal ASCII rendering
- Implemented four-wave pattern synthesis for complex cyberpunk visual generation
- Added hue-based neon gradient system with Cyan→Purple→Green→Cyan transitions
- Maintained <2GB memory usage and <800 draw calls for optimal performance

### Version 2.1.0 (2025-11-13)
- **MAJOR**: Added psychedelic DMT/acid trip visual generation with 35-50% probability triggers
- Enhanced AI Director with consciousness-expanding visual directives
- Updated role prompt with comprehensive psychedelic visual guidance
- Added fractal patterns, sacred geometries, and reality-bending transformations
- Maintained epilepsy safety constraints (2.0 Hz max flash rate)
- Implemented thread-safe random triggering mechanism

### Version 2.0.1 (2025-11-13)
- Aligned documentation with codebase class names
- Added Contribution Guide and License
- Refreshed documentation index and API reference

### Version 2.0.0 (2024-11-13)
- Restructured documentation according to style guide
- Added comprehensive project overview
- Updated performance targets and technical specifications
- Added cross-references to related documentation

### Version 1.0.0 (2024-01-01)
- Initial project documentation
- Basic project description and roadmap
