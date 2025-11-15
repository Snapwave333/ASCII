#pragma once

#include "NeonGlyph.h"
#include <vulkan/vulkan.h>
#include <string>
#include <memory>
#include <iostream>
#include <functional>

#if NEONGLYPH_HAVE_GLFW
#include <GLFW/glfw3.h>
#endif

namespace NeonGlyph {

class Window {
public:
    Window();
    ~Window();

    Result Create(const Config& config);
    void PollEvents();
    bool ShouldClose() const;

    Result CreateVulkanSurface(VkInstance instance, VkSurfaceKHR* surface);

    bool IsKeyPressed(int key) const;

    // Get native window handle for platform integration
    void* GetNativeHandle() const;
    
    // Set keyboard callback for custom key handling
    using KeyboardCallback = std::function<void(int key, int scancode, int action, int mods)>;
    void SetKeyboardCallback(KeyboardCallback callback);

    // Window mode management
    void SetBorderless(bool borderless);
    bool IsBorderless() const { return m_borderless; }
    
    void SetFullscreen(bool fullscreen);
    bool IsFullscreen() const { return m_fullscreen; }

    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
#if NEONGLYPH_HAVE_GLFW
    GLFWwindow* GetHandle() const { return m_window; }
#else
    void* GetHandle() const { return nullptr; }
#endif

private:
#if NEONGLYPH_HAVE_GLFW
    GLFWwindow* m_window;
#else
    void* m_window;
#endif
    uint32_t m_width;
    uint32_t m_height;
    std::string m_title;
    bool m_shouldClose;
    bool m_borderless;
    bool m_fullscreen;
    KeyboardCallback m_keyboardCallback;
};

} // namespace NeonGlyph