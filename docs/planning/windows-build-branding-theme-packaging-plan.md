# Windows Build, Branding, Theme, and Packaging Plan

**Document Type**: Technical Specification
**Version**: 1.0.0
**Last Updated**: 2025-11-16
**Status**: Draft
**Author**: NeonGlyph Development Team

## Build Executable
- Configure and build in a Visual Studio 2022 Developer Command Prompt using CMake and MSVC.
- Compiler/linker flags: `/O2` for speed or `/O1` for size, `/GL` + `/LTCG` for link-time optimization, `/Zc:__cplusplus`, `/MP`, strip debug info in Release.
- Runtime selection: `/MD` (dynamic CRT) with Visual C++ Redistributable bundled; optionally `/MT` for a self-contained binary when size is acceptable.
- Embed version info via a `.rc` resource (CompanyName, FileDescription, ProductName, ProductVersion, LegalCopyright).

## Branding Kit
- Name: "NeonGlyph Director" (trademarkable; aligns with existing namespace).
- Logo: vector glyph mark and wordmark; deliver `PNG` (multiple sizes), `SVG`, `ICO` (16/32/48/256).
- Color palette:
  - Primary: `#0FF0FC` (neon cyan), `#FF4D67` (neon magenta)
  - Neutrals: `#0D0D0D` (near-black), `#F2F2F2` (near-white)
  - Accents: `#FFE066` (warm accent), `#35FF69` (neon green)
- Typography:
  - UI: Segoe UI (Windows default) or Inter (fallback)
  - Monospace/rendered ASCII: Consolas
- Brand voice: visionary, cinematic, technical clarity; avoid jargon in user-facing text, emphasize "AI-directed performance".
- Deliver assets in `/assets/branding/` with a short style guide (palette, spacing, usage).

## Theme System
- Theme engine: JSON-driven templates under `config/themes/` (e.g., `light.json`, `dark.json`, `neon.json`).
- Schema fields: `palette` (foreground/background/accent), `ascii.brightness`, `ascii.contrast`, `glyph_map`, UI chrome colors.
- Auto-detect Windows light/dark via `HKCU\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize\AppsUseLightTheme` and apply on startup; allow manual override.
- Ensure consistency: Renderer and ASCIIConverter consume theme parameters; HUD text/images use theme palette.
- Hot-reload: watch theme files for live switching without restart.

## Windows Optimization & QoL
- Startup optimization:
  - Optional autostart entry at `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`.
  - Lightweight background helper for quick launch (defers heavy init until window created).
- System tray integration: `Shell_NotifyIcon` with context menu (Start/Stop, Theme toggle, Quit).
- Jump List support: `ICustomDestinationList` with tasks (Launch Performance Mode, Open Settings).
- Windows notifications: WinRT Toasts for status (audio device changes, performance alerts).
- High DPI: call `SetProcessDpiAwarenessContext`, create per-monitor DPI-aware window, scale ASCII cell size accordingly.
- Window management: proper minimize/restore, snapping, borderless fullscreen toggle.
- Installer/uninstaller: NSIS or WiX-based setup including registry/shortcuts; uninstaller cleans entries.

## Performance Optimization
- Lazy load: defer AI Director/Renderer heavy resources until first frame; load fonts/atlases on demand.
- Memory: reuse buffers, avoid per-frame allocations; preallocate instance buffers for ASCII grid.
- Startup profiling: measure time to first frame; cache shader modules; parallelize non-dependent init.
- Executable size: strip symbols, remove unused sections; consider resource compression for branding assets.

## Packaging & Distribution
- Signed installer: use `signtool.exe` with a code signing certificate; timestamp via a trusted TSA.
- Metadata: product name, versioning (Semantic Versioning), copyright, trademarks.
- Redistributables: bundle VC++ runtime (if `/MD`), Vulkan loader if needed; document dependencies.
- Optional MSIX package: prepare manifests for Windows Store submission.

## Implementation Steps
1. Add `.rc` resource for version info and icons; integrate in CMake target.
2. Create branding assets (SVG/PNG/ICO) and palette/typography guide.
3. Implement theme engine (JSON parsing, auto-detect, hot-reload) and wire into Renderer/ASCIIConverter.
4. Add Windows QoL features: tray icon, Jump List, notifications, DPI awareness.
5. Optimize build flags and memory; add simple startup profiler.
6. Create installer scripts (NSIS/WiX); implement signing and redistributable bundling.
7. Run build, test on a clean Windows VM, and verify installer/uninstaller behavior.

## Acceptance Criteria
- Release build produces a working `.exe` with version info and a signed installer.
- Light/dark themes apply automatically; user can switch themes; visuals remain consistent.
- Windows QoL features operate correctly; no orphaned registry entries post-uninstall.
- Performance: fast startup, smooth rendering; executable size minimized without breaking functionality.

## Contingencies
 - If code signing cert is unavailable, proceed unsigned and add signing later.
 - If NSIS/WiX is preferred by you, confirm choice; plan supports either.
 - If Jump List/Toast dependencies complicate the pipeline, gate them behind build options and ship core app first.

## Implementation Status
- System tray integration: Planned
- Jump Lists: Planned
- Toast notifications: Planned
- High DPI support: Planned
- Borderless fullscreen/window management: Planned
- Installer/uninstaller (NSIS/WiX): Planned
- Code signing: Planned
