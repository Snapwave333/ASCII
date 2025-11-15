#include "Renderer.h"
#include "VulkanContext.h"
#include "ASCIIConverter.h"
#include <nlohmann/json.hpp>
#include <utility>
#include <random>
#include <climits>
#include <iostream>
#include <cmath>
#include <exception>
#include "Logger.h"

namespace NeonGlyph {

// Safe string to float conversion with error handling
static float SafeStof(const std::string& str, float default_val = 0.0f) {
    try {
        if (str.empty()) return default_val;
        return std::stof(str);
    } catch (const std::exception& e) {
        std::cerr << "[Renderer] Warning: Failed to parse float value '" << str << "': " << e.what() << std::endl;
        return default_val;
    }
}

// Safe string to unsigned long conversion with error handling
static uint32 SafeStoul(const std::string& str, uint32 default_val = 0) {
    try {
        if (str.empty()) return default_val;
        return std::stoul(str);
    } catch (const std::exception& e) {
        std::cerr << "[Renderer] Warning: Failed to parse integer value '" << str << "': " << e.what() << std::endl;
        return default_val;
    }
}

// Safe string to integer conversion with error handling
static int SafeStoi(const std::string& str, int default_val = 0) {
    try {
        if (str.empty()) return default_val;
        return std::stoi(str);
    } catch (const std::exception& e) {
        std::cerr << "[Renderer] Warning: Failed to parse integer value '" << str << "': " << e.what() << std::endl;
        return default_val;
    }
}

Renderer::Renderer() : m_vulkanContext(nullptr), m_asciiConverter(nullptr) {}
Renderer::~Renderer() { Shutdown(); }

Result Renderer::Initialize(VulkanContext* context, const Config& config) {
    m_vulkanContext = context;
    m_config = config;
    ResizeCanvas(120, 48);
    ClearCanvas(' ');
    m_overlayEnabled = m_config.render.overlayEnabled;
    m_overlayPosition = m_config.render.overlayPosition;
    return Result::Success;
}

void Renderer::Shutdown() {
    while (!m_queue.empty()) m_queue.pop();
}

void Renderer::SetASCIIConverter(ASCIIConverter* converter) { m_asciiConverter = converter; }

void Renderer::SetOverlayData(const OverlayData& data) {
    m_overlay = data;
    m_overlayEnabled = data.enabled;
    if (!data.position.empty()) m_overlayPosition = data.position;
}

Result Renderer::Submit(const DirectorCommand& cmd) {
    m_queue.push(cmd);
    return Result::Success;
}

Result Renderer::Update(Duration dt) {
    static bool firstTick = false;
    if (!firstTick) { NeonGlyph::Logger::LogLine("SceneManager FirstTick"); firstTick = true; }
    if (m_queue.empty()) return Result::Success;
    DirectorCommand cmd = m_queue.front();
    m_queue.pop();
    ApplyColorMorph(cmd.audio_sync.energy, cmd.audio_sync.phase);
    for (const auto& d : cmd.directives) {
        if (d.name == "GenerateMaze") {
            uint32 w = 64, h = 32; float32 c = 0.3f;
            auto itW = d.params.find("width");
            auto itH = d.params.find("height");
            auto itC = d.params.find("complexity");
            if (itW != d.params.end()) w = SafeStoul(itW->second);
            if (itH != d.params.end()) h = SafeStoul(itH->second);
            if (itC != d.params.end()) c = SafeStof(itC->second);
            GenerateMaze(w, h, c);
        } else if (d.name == "SpawnCharacter") {
            auto itT = d.params.find("type");
            auto itP = d.params.find("position");
            std::string type = itT != d.params.end() ? itT->second : "default";
            std::string pos = itP != d.params.end() ? itP->second : "0,0";
            SpawnCharacter(type, pos);
        } else if (d.name == "AnimateFluidSim") {
            float32 s = 1.0f, v = 0.5f, b = 0.5f;
            auto itS = d.params.find("scale");
            auto itV = d.params.find("viscosity");
            auto itB = d.params.find("beat_coupling");
            if (itS != d.params.end()) s = SafeStof(itS->second);
            if (itV != d.params.end()) v = SafeStof(itV->second);
            if (itB != d.params.end()) b = SafeStof(itB->second);
            AnimateFluidSim(s, v, b);
        } else if (d.name == "RenderText") {
            auto itS = d.params.find("string");
            auto itF = d.params.find("font");
            auto itSt = d.params.find("style");
            std::string s = itS != d.params.end() ? itS->second : "";
            std::string f = itF != d.params.end() ? itF->second : "default";
            std::string st = itSt != d.params.end() ? itSt->second : "normal";
            RenderText(s, f, st);
        } else if (d.name == "RenderSceneSpec") {
            auto itSp = d.params.find("spec");
            std::string spec = itSp != d.params.end() ? itSp->second : "";
            RenderSceneSpec(spec);
        } else if (d.name == "RenderNeonGlyphLogo") {
            RenderNeonGlyphLogo();
        } else if (d.name == "MorphToTestCard") {
            auto itT = d.params.find("technique");
            std::string tech = itT != d.params.end() ? itT->second : "particle";
            MorphToTestCard(tech);
        } else if (d.name == "RenderTestCard") {
            auto itM = d.params.find("mode");
            std::string mode = itM != d.params.end() ? itM->second : "auto";
            RenderTestCard(mode);
        } else if (d.name == "SetPalette") {
            auto itP = d.params.find("primary");
            auto itS = d.params.find("secondary");
            auto itS2 = d.params.find("secondary2");
            auto itA = d.params.find("accent");
            auto itSh = d.params.find("shadow");
            auto itH = d.params.find("highlight");
            auto parse = [&](const std::string& hex){ std::string t=hex; if(!t.empty()&&t[0]=='#') t=t.substr(1); if(t.size()==6) t+="FF"; try{ return static_cast<uint32>(std::stoul(t, nullptr, 16)); }catch(...){ return 0xFFFFFFFFu; } };
            if (itP != d.params.end()) m_activePrimary = parse(itP->second);
            if (itS != d.params.end()) m_activeSecondary = parse(itS->second);
            if (itS2 != d.params.end()) m_baseSecondary2 = parse(itS2->second);
            if (itA != d.params.end()) m_activeAccent = parse(itA->second);
            if (itSh != d.params.end()) m_activeShadow = parse(itSh->second);
            if (itH != d.params.end()) m_activeHighlight = parse(itH->second);
            m_basePrimary = m_activePrimary;
            m_baseSecondary = m_activeSecondary;
            m_baseAccent = m_activeAccent;
            m_baseShadow = m_activeShadow;
            m_baseHighlight = m_activeHighlight;
            if (m_asciiConverter) {
                std::vector<uint32> cols;
                cols.push_back(m_activePrimary);
                cols.push_back(m_activeSecondary);
                cols.push_back(m_activeAccent);
                cols.push_back(m_activeShadow);
                cols.push_back(m_activeHighlight);
                ColorPalette cp("active", cols);
                m_asciiConverter->SetColorPalette(cp);
            }
        }
    }
    ComposeFrameString();
    return Result::Success;
}

Result Renderer::GenerateMaze(uint32 width, uint32 height, float32 complexity) {
    if (width < 8) width = 8;
    if (height < 8) height = 8;
    ResizeCanvas(width + 2, height + 2);
    ClearCanvas('#');
    std::vector<std::vector<int>> grid(height, std::vector<int>(width, 1));
    auto carve = [&](auto&& self, int x, int y) -> void {
        grid[y][x] = 0;
        std::vector<std::pair<int,int>> dirs = { {2,0}, {-2,0}, {0,2}, {0,-2} };
        std::shuffle(dirs.begin(), dirs.end(), std::mt19937{std::random_device{}()});
        for (auto& d : dirs) {
            int nx = x + d.first;
            int ny = y + d.second;
            if (nx > 0 && nx < width && ny > 0 && ny < height && grid[ny][nx] == 1) {
                int wx = x + d.first/2;
                int wy = y + d.second/2;
                grid[wy][wx] = 0;
                self(self, nx, ny);
            }
        }
    };
    carve(carve, 1, 1);
    DrawMazeGrid(grid);
    return Result::Success;
}

Result Renderer::SpawnCharacter(const std::string& type, const std::string& position) {
    int x = 1;
    int y = 1;
    size_t comma = position.find(',');
    if (comma != std::string::npos) {
        x = SafeStoi(position.substr(0, comma));
        y = SafeStoi(position.substr(comma + 1));
    }
    if (x >= 0 && x < static_cast<int>(m_canvasWidth) && y >= 0 && y < static_cast<int>(m_canvasHeight)) {
        m_canvas[y * m_canvasWidth + x] = '@';
    }
    return Result::Success;
}

Result Renderer::AnimateFluidSim(float32 scale, float32 viscosity, float32 beat_coupling) {
    for (uint32 y = 0; y < m_canvasHeight; ++y) {
        for (uint32 x = 0; x < m_canvasWidth; ++x) {
            float a = std::sin(static_cast<float>(x) * 0.1f * scale) + std::cos(static_cast<float>(y) * 0.13f * scale);
            char ch = a > 0.5f ? '~' : a < -0.5f ? '.' : ' ';
            m_canvas[y * m_canvasWidth + x] = ch;
        }
    }
    return Result::Success;
}

Result Renderer::RenderText(const std::string& text, const std::string& font, const std::string& style) {
    int x = static_cast<int>(m_canvasWidth / 2 - text.size() / 2);
    int y = static_cast<int>(m_canvasHeight / 2);
    DrawTextASCII(x, y, text);
    return Result::Success;
}

std::string Renderer::GetFrameString() const { return m_lastFrame; }

std::vector<uint8> Renderer::ComposeFramePixels(uint32 outWidth, uint32 outHeight) const {
    uint32 cw = m_canvasWidth;
    uint32 ch = m_canvasHeight;
    if (cw == 0 || ch == 0 || outWidth == 0 || outHeight == 0) return {};
    std::vector<uint8> pixels(static_cast<size_t>(outWidth) * outHeight * 4);
    static bool loggedPack = false;
    if (!loggedPack) { NeonGlyph::Logger::LogLine("PixelPack BGRA 8-8-8-8"); loggedPack = true; }
    for (uint32 y = 0; y < outHeight; ++y) {
        uint32 sy = (y * ch) / outHeight;
        for (uint32 x = 0; x < outWidth; ++x) {
            uint32 sx = (x * cw) / outWidth;
            size_t sidx = static_cast<size_t>(sy) * cw + sx;
            char c = m_canvas.empty() ? ' ' : m_canvas[sidx];
            uint32 fg = m_fgColor.empty() ? 0xFFFFFFFFu : m_fgColor[sidx];
            uint32 rgba = (c == ' ') ? m_config.render.startupBgColor : (0xFF000000u | (fg & 0x00FFFFFFu));
            if (((rgba >> 24) & 0xFFu) == 0u) rgba |= 0xFF000000u;
            size_t didx = (static_cast<size_t>(y) * outWidth + x) * 4;
            pixels[didx + 0] = static_cast<uint8>(rgba & 0xFFu);
            pixels[didx + 1] = static_cast<uint8>((rgba >> 8) & 0xFFu);
            pixels[didx + 2] = static_cast<uint8>((rgba >> 16) & 0xFFu);
            pixels[didx + 3] = static_cast<uint8>((rgba >> 24) & 0xFFu);
        }
    }
    return pixels;
}

void Renderer::ResizeCanvas(uint32 w, uint32 h) {
    m_canvasWidth = w;
    m_canvasHeight = h;
    m_canvas.assign(static_cast<size_t>(w) * h, ' ');
    m_fgColor.assign(static_cast<size_t>(w) * h, 0xFFFFFFFFu);
}

void Renderer::ClearCanvas(char ch) {
    std::fill(m_canvas.begin(), m_canvas.end(), ch);
    std::fill(m_fgColor.begin(), m_fgColor.end(), 0xFFFFFFFFu);
}

void Renderer::DrawTextASCII(int x, int y, const std::string& s) {
    if (y < 0 || y >= static_cast<int>(m_canvasHeight)) return;
    for (size_t i = 0; i < s.size(); ++i) {
        int px = x + static_cast<int>(i);
        if (px >= 0 && px < static_cast<int>(m_canvasWidth)) m_canvas[y * m_canvasWidth + px] = s[i];
    }
}

void Renderer::DrawMazeGrid(const std::vector<std::vector<int>>& grid) {
    for (size_t y = 0; y < grid.size(); ++y) {
        for (size_t x = 0; x < grid[y].size(); ++x) {
            char ch = grid[y][x] == 0 ? ' ' : '#';
            uint32 yy = static_cast<uint32>(y);
            uint32 xx = static_cast<uint32>(x);
            if (yy + 1u < m_canvasHeight && xx + 1u < m_canvasWidth) {
                m_canvas[(yy + 1u) * m_canvasWidth + (xx + 1u)] = ch;
            }
        }
    }
}

void Renderer::ComposeFrameString() {
    std::string s;
    s.reserve(static_cast<size_t>(m_canvasWidth + 10) * m_canvasHeight);
    uint32 prev = 0xFFFFFFFFu;
    for (uint32 y = 0; y < m_canvasHeight; ++y) {
        for (uint32 x = 0; x < m_canvasWidth; ++x) {
            size_t idx = static_cast<size_t>(y) * m_canvasWidth + x;
            uint32 c = m_fgColor.empty() ? 0xFFFFFFFFu : m_fgColor[idx];
            if (c != prev) {
                if (c == 0xFFFFFFFFu) {
                    s.append("\x1B[0m");
                } else if (m_colorMode == "truecolor") {
                    uint32 r = (c >> 16) & 0xFFu;
                    uint32 g = (c >> 8) & 0xFFu;
                    uint32 b = c & 0xFFu;
                    s.append("\x1B[38;2;");
                    s.append(std::to_string(r));
                    s.push_back(';');
                    s.append(std::to_string(g));
                    s.push_back(';');
                    s.append(std::to_string(b));
                    s.append("m");
                } else if (m_colorMode == "ansi") {
                    int idxAnsi = static_cast<int>(c & 0xFFu);
                    int base = idxAnsi < 8 ? 30 : 90;
                    int code = base + (idxAnsi % 8);
                    s.append("\x1B[");
                    s.append(std::to_string(code));
                    s.append("m");
                } else {
                    s.append("\x1B[0m");
                }
                prev = c;
            }
            s.push_back(m_canvas[idx]);
        }
        s.append("\x1B[0m\n");
        prev = 0xFFFFFFFFu;
    }
    m_lastFrame = std::move(s);
}

void Renderer::DrawOverlayToCanvas() {
    if (!m_overlayEnabled) return;
    std::string l1 = std::string("FPS=") + std::to_string(m_overlay.fps);
    std::string l2 = std::string("FrameMs=") + std::to_string(m_overlay.frameMs);
    std::string l3 = std::string("RenderMs=") + std::to_string(m_overlay.renderMs);
    std::string l4 = std::string("PresentMs=") + std::to_string(m_overlay.presentMs);
    std::string l5 = std::string("Jitter=") + std::to_string(m_overlay.jitter) + std::string(" DF=") + std::to_string(m_overlay.droppedFrames);
    std::string l6 = std::string("AudioRMS=") + std::to_string(m_overlay.audioRms)
                   + std::string(" Peak=") + std::to_string(m_overlay.audioPeak)
                   + std::string(" B=") + std::to_string(m_overlay.audioBass)
                   + std::string(" M=") + std::to_string(m_overlay.audioMids)
                   + std::string(" H=") + std::to_string(m_overlay.audioHighs);
    uint32 overlayWidth = 0;
    overlayWidth = std::max<uint32>(overlayWidth, static_cast<uint32>(l1.size()));
    overlayWidth = std::max<uint32>(overlayWidth, static_cast<uint32>(l2.size()));
    overlayWidth = std::max<uint32>(overlayWidth, static_cast<uint32>(l3.size()));
    overlayWidth = std::max<uint32>(overlayWidth, static_cast<uint32>(l4.size()));
    overlayWidth = std::max<uint32>(overlayWidth, static_cast<uint32>(l5.size()));
    overlayWidth = std::max<uint32>(overlayWidth, static_cast<uint32>(l6.size()));
    overlayWidth = std::min<uint32>(overlayWidth + 2u, m_canvasWidth);
    uint32 ox = 1;
    uint32 oy = 1;
    if (m_overlayPosition == "top_left") { ox = 1; oy = 1; }
    else if (m_overlayPosition == "top_right") { ox = m_canvasWidth > overlayWidth ? m_canvasWidth - overlayWidth : 0; oy = 1; }
    else if (m_overlayPosition == "bottom_left") { ox = 1; oy = m_canvasHeight > 7 ? m_canvasHeight - 7 : 0; }
    else if (m_overlayPosition == "bottom_right") { ox = m_canvasWidth > overlayWidth ? m_canvasWidth - overlayWidth : 0; oy = m_canvasHeight > 7 ? m_canvasHeight - 7 : 0; }
    auto drawLine = [&](uint32 y, const std::string& str){ for (size_t i=0;i<str.size() && (ox+i)<m_canvasWidth;i++){ size_t idx = static_cast<size_t>(y)*m_canvasWidth + (ox+static_cast<uint32>(i)); m_canvas[idx] = str[i]; m_fgColor[idx] = 0x00FF88u; } };
    uint32 y0 = oy;
    if (y0 < m_canvasHeight) drawLine(y0, l1);
    if (y0+1 < m_canvasHeight) drawLine(y0+1, l2);
    if (y0+2 < m_canvasHeight) drawLine(y0+2, l3);
    if (y0+3 < m_canvasHeight) drawLine(y0+3, l4);
    if (y0+4 < m_canvasHeight) drawLine(y0+4, l5);
    if (y0+5 < m_canvasHeight) drawLine(y0+5, l6);
}

void Renderer::UpdateOverlay() { DrawOverlayToCanvas(); }

static uint32 blendRGB(uint32 a, uint32 b, float t){ int ar=(a>>16)&255; int ag=(a>>8)&255; int ab=a&255; int br=(b>>16)&255; int bg=(b>>8)&255; int bb=b&255; int rr=static_cast<int>(ar+(br-ar)*t); int rg=static_cast<int>(ag+(bg-ag)*t); int rb=static_cast<int>(ab+(bb-ab)*t); return (static_cast<uint32>(rr&255)<<16)|(static_cast<uint32>(rg&255)<<8)|static_cast<uint32>(rb&255); }
static float lum(uint32 c){ float r=((c>>16)&255)/255.0f; float g=((c>>8)&255)/255.0f; float b=(c&255)/255.0f; return 0.299f*r+0.587f*g+0.114f*b; }

void Renderer::ApplyColorMorph(float32 energy, float32 phase) {
    float w = std::clamp(0.1f + 0.3f*energy + 0.1f*std::sin(phase*6.2831f), 0.0f, 0.5f);
    uint32 white = 0xFFFFFFu;
    uint32 dark = m_baseShadow & 0xFFFFFFu;
    uint32 p = blendRGB(m_basePrimary & 0xFFFFFFu, white, w);
    uint32 s = blendRGB(m_baseSecondary & 0xFFFFFFu, white, w*0.6f);
    uint32 a = blendRGB(m_baseAccent & 0xFFFFFFu, white, w*0.8f);
    uint32 sh = blendRGB(dark, m_basePrimary & 0xFFFFFFu, w*0.3f);
    uint32 hi = blendRGB(m_baseHighlight & 0xFFFFFFu, white, w);
    if (std::abs(lum(p)-lum(sh)) < 0.15f) { sh = blendRGB(dark, p, 0.2f); }
    m_activePrimary = p;
    m_activeSecondary = s;
    m_activeAccent = a;
    m_activeShadow = sh;
    m_activeHighlight = hi;
    m_lastMorphPhase = phase;
}

Result Renderer::RenderSceneSpec(const std::string& spec_json) {
    using nlohmann::json;
    if (spec_json.empty()) return Result::Error;
    auto j = json::parse(spec_json, nullptr, false);
    if (j.is_discarded()) return Result::Error;
    uint32 w = j.contains("canvas") && j["canvas"].contains("width") ? static_cast<uint32>(j["canvas"]["width"].get<int>()) : m_canvasWidth;
    uint32 h = j.contains("canvas") && j["canvas"].contains("height") ? static_cast<uint32>(j["canvas"]["height"].get<int>()) : m_canvasHeight;
    ResizeCanvas(w, h);
    ClearCanvas(' ');
    std::string sym = j.contains("style") && j["style"].contains("symmetry") ? j["style"]["symmetry"].get<std::string>() : std::string("none");
    bool panel_enabled = j.contains("style") && j["style"].contains("panelization") && j["style"]["panelization"].contains("enabled") ? j["style"]["panelization"]["enabled"].get<bool>() : false;
    int panel_cols = j.contains("style") && j["style"].contains("panelization") && j["style"]["panelization"].contains("columns") ? j["style"]["panelization"]["columns"].get<int>() : 1;
    int panel_rows = j.contains("style") && j["style"].contains("panelization") && j["style"]["panelization"].contains("rows") ? j["style"]["panelization"]["rows"].get<int>() : 1;
    auto layers = j.contains("layers") ? j["layers"] : json::array();
    float phase = j.contains("motion") && j["motion"].contains("phase_0_1") ? j["motion"]["phase_0_1"].get<float>() : 0.0f;
    float bass = j.contains("audio") && j["audio"].contains("bass_level") && !j["audio"]["bass_level"].is_null() ? j["audio"]["bass_level"].get<float>() : 0.0f;
    auto color = j.contains("color") ? j["color"] : json::object();
    std::string mode = color.contains("mode") ? color["mode"].get<std::string>() : std::string("mono");
    m_colorMode = mode == "truecolor" ? "truecolor" : mode == "ansi" ? "ansi" : "mono";
    std::cout << "ColorMode=" << m_colorMode << std::endl;
    auto readRGB = [&](const json& arr) -> uint32 {
        if (!arr.is_array() || arr.size() < 3) return 0xFFFFFFFFu;
        int r = arr[0].get<int>(); int g = arr[1].get<int>(); int b = arr[2].get<int>();
        return (static_cast<uint32>(r & 255) << 16) | (static_cast<uint32>(g & 255) << 8) | static_cast<uint32>(b & 255);
    };
    auto nearestAnsi = [&](uint32 rgb) -> uint32 {
        static const uint32 ansiRGB[16] = {
            0x000000u,0x800000u,0x008000u,0x808000u,0x000080u,0x800080u,0x008080u,0xC0C0C0u,
            0x808080u,0xFF0000u,0x00FF00u,0xFFFF00u,0x0000FFu,0xFF00FFu,0x00FFFFu,0xFFFFFFu
        };
        auto dr = [&](uint32 a,uint32 b){int ar=(a>>16)&255;int ag=(a>>8)&255;int ab=a&255;int br=(b>>16)&255;int bg=(b>>8)&255;int bb=b&255;int dx=ar-br;int dy=ag-bg;int dz=ab-bb;return dx*dx+dy*dy+dz*dz;};
        int best=0;int bestd=INT_MAX;for(int i=0;i<16;++i){int d=dr(rgb,ansiRGB[i]);if(d<bestd){bestd=d;best=i;}}return static_cast<uint32>(best);
    };
    uint32 primary = color.contains("primary") ? readRGB(color["primary"]) : (m_activePrimary & 0xFFFFFFu);
    uint32 shadow = color.contains("shadow") ? readRGB(color["shadow"]) : (m_activeShadow & 0xFFFFFFu);
    uint32 highlight = color.contains("highlights") ? readRGB(color["highlights"]) : (m_activeHighlight & 0xFFFFFFu);
    uint32 secondary0 = (m_colorMode=="truecolor"? (m_activeSecondary & 0xFFFFFFu) : 0x808080u);
    if (color.contains("secondary") && color["secondary"].is_array() && !color["secondary"].empty()) {
        secondary0 = readRGB(color["secondary"][0]);
    }
    auto setColor = [&](uint32 x,uint32 y,uint32 rgb){
        size_t idx = static_cast<size_t>(y) * m_canvasWidth + x;
        if (idx < m_fgColor.size()) {
            if (m_colorMode == "truecolor") m_fgColor[idx] = rgb;
            else if (m_colorMode == "ansi") m_fgColor[idx] = nearestAnsi(rgb);
            else m_fgColor[idx] = 0xFFFFFFFFu;
        }
    };
    for (auto& layer : layers) {
        std::string name = layer.contains("name") ? layer["name"].get<std::string>() : std::string("background");
        std::string charset = layer.contains("char_set") ? layer["char_set"].get<std::string>() : std::string(" .:-");
        float density = layer.contains("density") ? layer["density"].get<float>() : 0.1f;
        std::string motif = layer.contains("motif") ? layer["motif"].get<std::string>() : std::string("soft_noise");
        if (name == "background") {
            for (uint32 y = 0; y < m_canvasHeight; ++y) {
                for (uint32 x = 0; x < m_canvasWidth; ++x) {
                    float fx = static_cast<float>(x) / static_cast<float>(m_canvasWidth);
                    float fy = static_cast<float>(y) / static_cast<float>(m_canvasHeight);
                    float v = 0.0f;
                    if (motif == "starfield") v = static_cast<float>((x * 73856093u ^ y * 19349663u) & 255) / 255.0f;
                    else if (motif == "slow_waves") v = (std::sin(fx * 6.2831f + phase * 6.2831f) + std::cos(fy * 6.2831f)) * 0.25f + 0.5f;
                    else v = static_cast<float>((x * 2654435761u + y * 97531u) & 255) / 255.0f;
                    if (v < density) { m_canvas[y * m_canvasWidth + x] = charset[static_cast<size_t>(v * (charset.size() - 1))]; uint32 rgb = blendRGB(shadow, primary, 0.25f + 0.25f*phase); setColor(x,y,rgb);} 
                }
            }
        } else if (name == "midground") {
            if (motif == "grid") {
                int gx = std::max(2, static_cast<int>(m_canvasWidth / 16));
                int gy = std::max(2, static_cast<int>(m_canvasHeight / 8));
                for (uint32 y = 0; y < m_canvasHeight; y += gy) {
                    for (uint32 x = 0; x < m_canvasWidth; ++x) { m_canvas[y * m_canvasWidth + x] = charset[(x + y) % charset.size()]; uint32 rgb = blendRGB(secondary0, highlight, 0.1f*std::sin(phase*6.2831f)+0.1f); setColor(x,y,rgb);} 
                }
                for (uint32 x = 0; x < m_canvasWidth; x += gx) {
                    for (uint32 y = 0; y < m_canvasHeight; ++y) { m_canvas[y * m_canvasWidth + x] = charset[(x + y) % charset.size()]; uint32 rgb = blendRGB(secondary0, highlight, 0.1f*std::sin(phase*6.2831f)+0.1f); setColor(x,y,rgb);} 
                }
            } else if (motif == "tunnels") {
                uint32 cx = m_canvasWidth / 2, cy = m_canvasHeight / 2;
                uint32 steps = std::min(m_canvasWidth, m_canvasHeight) / 4;
                for (uint32 s = 1; s < steps; ++s) {
                    uint32 hw = static_cast<uint32>(s * 2);
                    uint32 hh = static_cast<uint32>(s);
                    uint32 x0 = cx - hw, x1 = cx + hw;
                    uint32 y0 = cy - hh, y1 = cy + hh;
                    if (x0 >= m_canvasWidth || y0 >= m_canvasHeight) continue;
                    for (uint32 x = x0; x < std::min(x1, m_canvasWidth); ++x) {
                        float t = static_cast<float>(s)/static_cast<float>(steps);
                        uint32 cA = blendRGB(secondary0, primary, t*0.3f);
                        if (y0 < m_canvasHeight) { m_canvas[y0 * m_canvasWidth + x] = charset[s % charset.size()]; setColor(x,y0,cA);} 
                        if (y1 < m_canvasHeight) { m_canvas[y1 * m_canvasWidth + x] = charset[s % charset.size()]; setColor(x,y1,cA);} 
                    }
                    for (uint32 y = y0; y < std::min(y1, m_canvasHeight); ++y) {
                        float t = static_cast<float>(s)/static_cast<float>(steps);
                        uint32 cB = blendRGB(secondary0, highlight, t*0.2f);
                        if (x0 < m_canvasWidth) { m_canvas[y * m_canvasWidth + x0] = charset[s % charset.size()]; setColor(x0,y,cB);} 
                        if (x1 < m_canvasWidth) { m_canvas[y * m_canvasWidth + x1] = charset[s % charset.size()]; setColor(x1,y,cB);} 
                    }
                }
            }
        } else if (name == "foreground") {
            bool has_totem = j.contains("focus") && j["focus"].contains("has_central_totem") ? j["focus"]["has_central_totem"].get<bool>() : false;
            float wf = j.contains("focus") && j["focus"].contains("totem_width_fraction") ? j["focus"]["totem_width_fraction"].get<float>() : 0.15f;
            if (has_totem) DrawTotem(wf, charset);
            uint32 cx = m_canvasWidth / 2;
            for (uint32 y = 0; y < m_canvasHeight; ++y) {
                if (bass > 0.5f && y % 4 == 0) {
                    float pulse = std::clamp((bass-0.5f)*2.0f, 0.0f, 1.0f);
                    uint32 beam = blendRGB(m_activeAccent & 0xFFFFFFu, highlight, 0.5f*pulse);
                    if (cx < m_canvasWidth) { m_canvas[y * m_canvasWidth + cx] = charset.back(); setColor(cx,y,beam);} 
                }
            }
            if (has_totem) {
                uint32 width = std::max<uint32>(1, static_cast<uint32>(static_cast<float>(m_canvasWidth) * wf));
                uint32 x0 = (m_canvasWidth - width) / 2;
                for (uint32 y = 0; y < m_canvasHeight; ++y) {
                    for (uint32 x = x0; x < x0 + width && x < m_canvasWidth; ++x) setColor(x,y,primary);
                }
            }
        }
    }
    if (panel_enabled) {
        int cw = std::max(1, static_cast<int>(m_canvasWidth / std::max(1, panel_cols)));
        int ch = std::max(1, static_cast<int>(m_canvasHeight / std::max(1, panel_rows)));
        for (int py = 0; py < panel_rows; ++py) {
            for (int px = 0; px < panel_cols; ++px) {
                bool edge = ((px + py) % 2) == 0;
                uint32 x0 = static_cast<uint32>(px * cw);
                uint32 y0 = static_cast<uint32>(py * ch);
                for (uint32 x = x0; x < std::min<uint32>(x0 + cw, m_canvasWidth); ++x) {
                    if (y0 < m_canvasHeight) m_canvas[y0 * m_canvasWidth + x] = edge ? '-' : '=';
                }
                for (uint32 y = y0; y < std::min<uint32>(y0 + ch, m_canvasHeight); ++y) {
                    if (x0 < m_canvasWidth) m_canvas[y * m_canvasWidth + x0] = edge ? '|' : '+';
                }
            }
        }
    }
    ApplySymmetry(sym);
    return Result::Success;
}

void Renderer::ApplySymmetry(const std::string& mode) {
    if (mode == "vertical_mirror") {
        for (uint32 y = 0; y < m_canvasHeight; ++y) {
            for (uint32 x = 0; x < m_canvasWidth / 2; ++x) {
                m_canvas[y * m_canvasWidth + (m_canvasWidth - 1 - x)] = m_canvas[y * m_canvasWidth + x];
            }
        }
    } else if (mode == "horizontal_mirror") {
        for (uint32 y = 0; y < m_canvasHeight / 2; ++y) {
            for (uint32 x = 0; x < m_canvasWidth; ++x) {
                m_canvas[(m_canvasHeight - 1 - y) * m_canvasWidth + x] = m_canvas[y * m_canvasWidth + x];
            }
        }
    } else if (mode == "radial_4") {
        uint32 hw = m_canvasWidth / 2;
        uint32 hh = m_canvasHeight / 2;
        for (uint32 y = 0; y < hh; ++y) {
            for (uint32 x = 0; x < hw; ++x) {
                char v = m_canvas[y * m_canvasWidth + x];
                m_canvas[y * m_canvasWidth + (m_canvasWidth - 1 - x)] = v;
                m_canvas[(m_canvasHeight - 1 - y) * m_canvasWidth + x] = v;
                m_canvas[(m_canvasHeight - 1 - y) * m_canvasWidth + (m_canvasWidth - 1 - x)] = v;
            }
        }
    }
}

void Renderer::DrawTotem(float32 width_fraction, const std::string& charset) {
    uint32 width = std::max<uint32>(1, static_cast<uint32>(static_cast<float>(m_canvasWidth) * width_fraction));
    uint32 x0 = (m_canvasWidth - width) / 2;
    for (uint32 y = 0; y < m_canvasHeight; ++y) {
        for (uint32 x = x0; x < x0 + width && x < m_canvasWidth; ++x) {
            m_canvas[y * m_canvasWidth + x] = charset[(y + x) % charset.size()];
        }
    }
}

void Renderer::SetCell(uint32 x, uint32 y, char ch, uint32 rgb) {
    if (x < m_canvasWidth && y < m_canvasHeight) {
        m_canvas[y * m_canvasWidth + x] = ch;
        if (!m_fgColor.empty()) m_fgColor[y * m_canvasWidth + x] = rgb;
    }
}

void Renderer::DrawHorizontalBar(uint32 y, uint32 x0, uint32 x1, char ch, uint32 rgb) {
    if (y >= m_canvasHeight) return;
    uint32 xa = x0 < x1 ? x0 : x1;
    uint32 xb = x0 < x1 ? x1 : x0;
    if (xa >= m_canvasWidth) return;
    if (xb > m_canvasWidth) xb = m_canvasWidth;
    for (uint32 x = xa; x < xb; ++x) SetCell(x, y, ch, rgb);
}

void Renderer::DrawVerticalBar(uint32 x, uint32 y0, uint32 y1, char ch, uint32 rgb) {
    if (x >= m_canvasWidth) return;
    uint32 ya = y0 < y1 ? y0 : y1;
    uint32 yb = y0 < y1 ? y1 : y0;
    if (ya >= m_canvasHeight) return;
    if (yb > m_canvasHeight) yb = m_canvasHeight;
    for (uint32 y = ya; y < yb; ++y) SetCell(x, y, ch, rgb);
}

Result Renderer::RenderNeonGlyphLogo() {
    ClearCanvas(' ');
    std::string s = "NEONGLYPH";
    int x = static_cast<int>(m_canvasWidth / 2 - s.size() / 2);
    int y = static_cast<int>(m_canvasHeight / 2);
    DrawTextASCII(x, y, s);
    for (uint32 i = 0; i < m_canvasWidth; ++i) SetCell(i, static_cast<uint32>(y - 2), '-', (m_colorMode=="truecolor"? ((m_activeAccent)&0xFFFFFFu) : 0xFFFFFFFFu));
    ApplySymmetry("vertical_mirror");
    ComposeFrameString();
    return Result::Success;
}

Result Renderer::MorphToTestCard(const std::string& technique) {
    if (technique == "particle") {
        for (uint32 y = 0; y < m_canvasHeight; ++y) {
            for (uint32 x = 0; x < m_canvasWidth; ++x) {
                uint32 r = static_cast<uint32>((x * 1103515245u + y * 12345u) & 0xFFu);
                char ch = " .:-=+*%#@"[r % 10];
                SetCell(x, y, ch, 0xFFFFFFFFu);
            }
        }
    } else if (technique == "fold") {
        ApplySymmetry("radial_4");
    } else if (technique == "beam") {
        for (uint32 y = 0; y < m_canvasHeight; y += 3) DrawHorizontalBar(y, 0, m_canvasWidth, '=', 0xFFFFFFFFu);
        for (uint32 x = 0; x < m_canvasWidth; x += 7) DrawVerticalBar(x, 0, m_canvasHeight, '|', 0xFFFFFFFFu);
    } else if (technique == "panel") {
        int cols = 4;
        int rows = 3;
        int cw = static_cast<int>(m_canvasWidth / cols);
        int ch = static_cast<int>(m_canvasHeight / rows);
        for (int py = 0; py < rows; ++py) {
            for (int px = 0; px < cols; ++px) {
                uint32 x0 = static_cast<uint32>(px * cw);
                uint32 y0 = static_cast<uint32>(py * ch);
                for (uint32 x = x0; x < x0 + static_cast<uint32>(cw) && x < m_canvasWidth; ++x) SetCell(x, y0, '-', 0xFFFFFFFFu);
                for (uint32 y = y0; y < y0 + static_cast<uint32>(ch) && y < m_canvasHeight; ++y) SetCell(x0, y, '|', 0xFFFFFFFFu);
            }
        }
    } else if (technique == "glitch") {
        for (uint32 y = 0; y < m_canvasHeight; ++y) {
            for (uint32 x = 0; x < m_canvasWidth; ++x) {
                char ch = (x + y) % 7 == 0 ? '#' : (x ^ y) % 5 == 0 ? '%' : ' ';
                SetCell(x, y, ch, 0xFFFFFFFFu);
            }
        }
    } else if (technique == "portal") {
        uint32 cx = m_canvasWidth / 2;
        uint32 cy = m_canvasHeight / 2;
        for (uint32 r = 2; r < m_canvasHeight / 2; r += 2) {
            for (int a = 0; a < 360; a += 6) {
                float rad = static_cast<float>(a) * 3.1415926f / 180.0f;
                uint32 x = static_cast<uint32>(cx + r * std::cos(rad));
                uint32 y = static_cast<uint32>(cy + r * std::sin(rad));
                SetCell(x, y, '@', 0xFFFFFFFFu);
            }
        }
    }
    ComposeFrameString();
    return Result::Success;
}

Result Renderer::RenderTestCard(const std::string& mode) {
    ClearCanvas(' ');
    uint32 gridColor = (m_colorMode=="truecolor"? (m_activeShadow&0xFFFFFFu) : 0x808080u);
    for (uint32 y = 0; y < m_canvasHeight; y += 4) DrawHorizontalBar(y, 0, m_canvasWidth, '-', gridColor);
    for (uint32 x = 0; x < m_canvasWidth; x += 8) DrawVerticalBar(x, 0, m_canvasHeight, '|', gridColor);
    std::string ramp = " .:-=+*#%@";
    uint32 barH = 2;
    uint32 yBase = 2;
    for (size_t i = 0; i < ramp.size(); ++i) {
        uint32 y0 = yBase + static_cast<uint32>(i * (barH + 1));
        for (uint32 y = y0; y < y0 + barH && y < m_canvasHeight; ++y) {
            for (uint32 x = 2; x < m_canvasWidth / 3; ++x) SetCell(x, y, ramp[i], 0xFFFFFFFFu);
        }
    }
    bool useColor = m_colorMode == "truecolor" || m_colorMode == "ansi";
    uint32 colors[8] = {0xFF0000u,0x00FF00u,0x0000FFu,0xFFFF00u,0x00FFFFu,0xFF00FFu,0xFFFFFFu,0x808080u};
    uint32 yC = yBase;
    uint32 xStart = m_canvasWidth / 3 + 2;
    uint32 segW = static_cast<uint32>((m_canvasWidth - xStart - 2) / 8);
    for (int c = 0; c < 8; ++c) {
        for (uint32 y = yC + static_cast<uint32>(c * 3); y < yC + static_cast<uint32>(c * 3) + 2 && y < m_canvasHeight; ++y) {
            for (uint32 x = xStart + static_cast<uint32>(c * segW); x < xStart + static_cast<uint32>((c + 1) * segW) && x < m_canvasWidth; ++x) {
                char ch = '#';
                uint32 rgb = useColor ? ((c==0? (m_activePrimary&0xFFFFFFu) : c==1? (m_activeSecondary&0xFFFFFFu) : c==2? (m_activeAccent&0xFFFFFFu) : colors[c])) : 0xFFFFFFFFu;
                SetCell(x, y, ch, rgb);
            }
        }
    }
    uint32 mx0 = m_canvasWidth / 2 + 2;
    uint32 my0 = m_canvasHeight / 2 - 6;
    for (uint32 t = 0; t < 10; ++t) {
        DrawHorizontalBar(my0 + t, mx0, mx0 + 12, '-', (m_colorMode=="truecolor"? (m_activeHighlight&0xFFFFFFu) : 0xFFFFFFFFu));
    }
    for (uint32 t = 0; t < 6; ++t) {
        DrawVerticalBar(mx0 + 6, my0, my0 + 10, '|', (m_colorMode=="truecolor"? (m_activePrimary&0xFFFFFFu) : 0xFFFFFFFFu));
    }
    for (uint32 y = my0 + 12; y < my0 + 18 && y < m_canvasHeight; ++y) {
        for (uint32 x = mx0; x < mx0 + 12 && x < m_canvasWidth; ++x) SetCell(x, y, (x + y) % 2 ? '*' : ' ', (m_colorMode=="truecolor"? (m_activeSecondary&0xFFFFFFu) : 0xFFFFFFFFu));
    }
    uint32 pxPanels = 6;
    uint32 pw = m_canvasWidth / pxPanels;
    uint32 py = m_canvasHeight - 6;
    for (uint32 p = 0; p < pxPanels; ++p) {
        for (uint32 x = p * pw; x < (p + 1) * pw && x < m_canvasWidth; ++x) SetCell(x, py, '=', 0xFFFFFFFFu);
        char fill = p % 3 == 0 ? '#' : p % 3 == 1 ? '+' : '.';
        for (uint32 y = py + 1; y < py + 4 && y < m_canvasHeight; ++y) {
            for (uint32 x = p * pw + 1; x < (p + 1) * pw - 1 && x < m_canvasWidth; ++x) SetCell(x, y, fill, 0xFFFFFFFFu);
        }
    }
    std::string ghost = "NEONGLYPH";
    int gx = 2;
    int gy = static_cast<int>(m_canvasHeight - 2);
    DrawTextASCII(gx, gy, ghost);
    ComposeFrameString();
    return Result::Success;
}

} // namespace NeonGlyph
