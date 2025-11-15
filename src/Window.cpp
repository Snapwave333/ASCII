#include "Window.h"
#include "NeonGlyph.h"

#if NEONGLYPH_HAVE_GLFW
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <stdexcept>
#include <iostream>
#include <chrono>
#include "Logger.h"

namespace NeonGlyph {

Window::Window() 
    : m_window(nullptr)
    , m_width(1280)
    , m_height(720)
    , m_title("NeonGlyph - AI-Driven ASCII Visual Synthesis Engine")
    , m_shouldClose(false)
    , m_borderless(false)
    , m_fullscreen(false) {
}

Window::~Window() {
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
}

Result Window::Create(const Config& config) {
    auto t0 = std::chrono::high_resolution_clock::now();
    std::cout << "[Window] Starting window creation..." << std::endl;
    NeonGlyph::Logger::LogLine("Window Create start");
    
    std::cout << "[Window] Initializing GLFW..." << std::endl;
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return Result::InitializationFailed;
    }
    std::cout << "[Window] GLFW initialized successfully" << std::endl;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_width = config.window.width;
    m_height = config.window.height;
    m_title = "NeonGlyph - AI-Driven ASCII Visual Synthesis Engine";

    std::cout << "[Window] Creating GLFW window (" << m_width << "x" << m_height << ")..." << std::endl;
    m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return Result::InitializationFailed;
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    NeonGlyph::Logger::LogEvent("Window", "Create", ms);

    glfwSetWindowUserPointer(m_window, this);

    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, int width, int height) {
        auto appWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (appWindow) {
            appWindow->m_width = width;
            appWindow->m_height = height;
        }
    });

    glfwSetWindowCloseCallback(m_window, [](GLFWwindow* window) {
        auto appWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (appWindow) {
            appWindow->m_shouldClose = true;
        }
    });

    // Set up keyboard callback
    glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        auto appWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (appWindow && appWindow->m_keyboardCallback) {
            appWindow->m_keyboardCallback(key, scancode, action, mods);
        }
    });

    // Set up mouse button callback for double-click detection
    glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int button, int action, int mods) {
        auto appWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (appWindow && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            // Check for double-click (we'll implement proper double-click detection in the integration)
            static auto lastClickTime = std::chrono::steady_clock::now();
            auto currentTime = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastClickTime);
            
            if (duration.count() < 500) { // 500ms double-click threshold
                // Double-click detected - toggle fullscreen
                // This will be handled by the WindowsIntegration class
            }
            
            lastClickTime = currentTime;
        }
    });

    return Result::Success;
}

void Window::PollEvents() {
    auto t0 = std::chrono::high_resolution_clock::now();
    glfwPollEvents();
    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    NeonGlyph::Logger::LogEvent("Window", "PollEvents", ms);
}

bool Window::ShouldClose() const {
    int v = m_window ? glfwWindowShouldClose(m_window) : 1;
    NeonGlyph::Logger::LogLine(std::string("Window ShouldClose=") + std::to_string(v));
    return m_shouldClose || v;
}

Result Window::CreateVulkanSurface(VkInstance instance, VkSurfaceKHR* surface) {
    if (!m_window || !instance || !surface) {
        return Result::InvalidArgument;
    }

    auto t0 = std::chrono::high_resolution_clock::now();
    if (glfwCreateWindowSurface(instance, m_window, nullptr, surface) != VK_SUCCESS) {
        std::cerr << "Failed to create Vulkan surface" << std::endl;
        return Result::InitializationFailed;
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    NeonGlyph::Logger::LogEvent("Window", "CreateVulkanSurface", ms);

    return Result::Success;
}

bool Window::IsKeyPressed(int key) const {
    if (!m_window) return false;
    return glfwGetKey(m_window, key) == GLFW_PRESS;
}

void* Window::GetNativeHandle() const {
#if NEONGLYPH_HAVE_GLFW
    if (!m_window) return nullptr;
    return glfwGetWin32Window(m_window);
#else
    return nullptr;
#endif
}

void Window::SetKeyboardCallback(KeyboardCallback callback) {
    m_keyboardCallback = callback;
}

void Window::SetBorderless(bool borderless) {
    m_borderless = borderless;
#if NEONGLYPH_HAVE_GLFW
    if (m_window) {
        // This would require platform-specific code to actually change the window style
        // For now, we'll store the state and let the integration handle it
    }
#endif
}

void Window::SetFullscreen(bool fullscreen) {
    m_fullscreen = fullscreen;
#if NEONGLYPH_HAVE_GLFW
    if (m_window) {
        if (fullscreen) {
            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        } else {
            glfwSetWindowMonitor(m_window, nullptr, 0, 0, m_width, m_height, GLFW_DONT_CARE);
        }
    }
#endif
}

} // namespace NeonGlyph

#else

#include <iostream>

namespace NeonGlyph {

Window::Window() 
    : m_window(nullptr)
    , m_width(1280)
    , m_height(720)
    , m_title("NeonGlyph - AI-Driven ASCII Visual Synthesis Engine")
    , m_shouldClose(true)
    , m_borderless(false)
    , m_fullscreen(false) {
}

Window::~Window() {}

Result Window::Create(const Config& config) {
    (void)config;
    std::cerr << "GLFW not available - window creation disabled" << std::endl;
    return Result::UnsupportedOperation;
}

void Window::PollEvents() {}

bool Window::ShouldClose() const { return m_shouldClose; }

Result Window::CreateVulkanSurface(VkInstance instance, VkSurfaceKHR* surface) {
    (void)instance; (void)surface;
    std::cerr << "Vulkan surface creation not supported without GLFW" << std::endl;
    return Result::UnsupportedOperation;
}

bool Window::IsKeyPressed(int key) const { (void)key; return false; }

void* Window::GetNativeHandle() const { return nullptr; }

void Window::SetKeyboardCallback(KeyboardCallback callback) { (void)callback; }

void Window::SetBorderless(bool borderless) { (void)borderless; }

void Window::SetFullscreen(bool fullscreen) { (void)fullscreen; }

} // namespace NeonGlyph

#endif
