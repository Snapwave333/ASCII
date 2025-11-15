#pragma once

#include "NeonGlyph.h"
#include "DirectorCommand.h"
#include <string>
#include <queue>

namespace NeonGlyph {

class VulkanContext;
class ASCIIConverter;

class Renderer {
public:
    Renderer();
    ~Renderer();

    Result Initialize(VulkanContext* context, const Config& config);
    void Shutdown();

    void SetASCIIConverter(ASCIIConverter* converter);

    Result Submit(const DirectorCommand& cmd);
    Result Update(Duration dt);

    std::string GetFrameString() const;
    std::vector<uint8> ComposeFramePixels(uint32 outWidth, uint32 outHeight) const;
    void SetOverlayData(const OverlayData& data);
    void UpdateOverlay();

    Result GenerateMaze(uint32 width, uint32 height, float32 complexity);
    Result SpawnCharacter(const std::string& type, const std::string& position);
    Result AnimateFluidSim(float32 scale, float32 viscosity, float32 beat_coupling);
    Result RenderText(const std::string& text, const std::string& font, const std::string& style);
    Result RenderSceneSpec(const std::string& spec_json);
    Result RenderNeonGlyphLogo();
    Result MorphToTestCard(const std::string& technique);
    Result RenderTestCard(const std::string& mode);

private:
    VulkanContext* m_vulkanContext;
    ASCIIConverter* m_asciiConverter;
    Config m_config;
    std::queue<DirectorCommand> m_queue;

    uint32 m_canvasWidth = 80;
    uint32 m_canvasHeight = 40;
    std::vector<char> m_canvas;
    std::vector<uint32> m_fgColor;
    std::string m_colorMode;
    std::string m_lastFrame;
    uint32 m_activePrimary = 0xFFFFFFFFu;
    uint32 m_activeSecondary = 0xFFFFFFFFu;
    uint32 m_activeAccent = 0xFFFFFFFFu;
    uint32 m_activeShadow = 0x202020u;
    uint32 m_activeHighlight = 0xFFFFFFFFu;
    uint32 m_basePrimary = 0xFFFFFFFFu;
    uint32 m_baseSecondary = 0xFFFFFFFFu;
    uint32 m_baseSecondary2 = 0xFFFFFFFFu;
    uint32 m_baseAccent = 0xFFFFFFFFu;
    uint32 m_baseShadow = 0x202020u;
    uint32 m_baseHighlight = 0xFFFFFFFFu;
    float m_lastMorphPhase = 0.0f;
    void ApplyColorMorph(float32 energy, float32 phase);

    void ResizeCanvas(uint32 w, uint32 h);
    void ClearCanvas(char ch);
    void DrawTextASCII(int x, int y, const std::string& s);
    void DrawMazeGrid(const std::vector<std::vector<int>>& grid);
    void ComposeFrameString();
    void ApplySymmetry(const std::string& mode);
    void DrawTotem(float32 width_fraction, const std::string& charset);
    void SetCell(uint32 x, uint32 y, char ch, uint32 rgb);
    void DrawHorizontalBar(uint32 y, uint32 x0, uint32 x1, char ch, uint32 rgb);
    void DrawVerticalBar(uint32 x, uint32 y0, uint32 y1, char ch, uint32 rgb);
    OverlayData m_overlay;
    bool m_overlayEnabled = true;
    std::string m_overlayPosition = "top_left";
    void DrawOverlayToCanvas();
};

} // namespace NeonGlyph
