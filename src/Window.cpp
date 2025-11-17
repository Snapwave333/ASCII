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
    , m_fullscreen(false)
    , m_minimized(false) {
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
    
    // Enhanced error detection and logging
    std::cout << "[Window] Initializing GLFW..." << std::endl;
    if (!glfwInit()) {
        std::cerr << "[Window] CRITICAL: Failed to initialize GLFW" << std::endl;
        std::cerr << "[Window] This may indicate missing GLFW libraries or display server issues" << std::endl;
        
        // Log detailed error information
        NeonGlyph::Logger::LogLine("Window Create failed: GLFW initialization failed");
        
        // Check for common issues
        #ifdef _WIN32
        std::cerr << "[Window] Windows: Check if Microsoft Visual C++ Redistributables are installed" << std::endl;
        #elif __linux__
        std::cerr << "[Window] Linux: Check if X11/Wayland display server is running" << std::endl;
        std::cerr << "[Window] Linux: Try running with DISPLAY=:0 or use --headless mode" << std::endl;
        #elif __APPLE__
        std::cerr << "[Window] macOS: Check if Quartz display services are available" << std::endl;
        #endif
        
        return Result::InitializationFailed;
    }
    std::cout << "[Window] GLFW initialized successfully" << std::endl;

    // Set window hints for better compatibility
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    
    // Enhanced error detection for headless environments
    if (config.headless.enabled) {
        std::cout << "[Window] Headless mode detected, creating invisible window for Vulkan surface..." << std::endl;
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    }

    m_width = config.window.width;
    m_height = config.window.height;
    m_title = "NeonGlyph - AI-Driven ASCII Visual Synthesis Engine";

    std::cout << "[Window] Creating GLFW window (" << m_width << "x" << m_height << ")..." << std::endl;
    
    // Enhanced window creation with better error handling
    GLFWmonitor* monitor = nullptr;
    if (config.window.fullscreen) {
        monitor = glfwGetPrimaryMonitor();
        if (!monitor) {
            std::cerr << "[Window] WARNING: No primary monitor detected, falling back to windowed mode" << std::endl;
            monitor = nullptr;
        }
    }
    
    m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), monitor, nullptr);
    if (!m_window) {
        std::cerr << "[Window] CRITICAL: Failed to create GLFW window" << std::endl;
        
        // Enhanced error detection and reporting
        const char* error_description;
        int error_code = glfwGetError(&error_description);
        
        std::cerr << "[Window] GLFW Error Code: " << error_code << std::endl;
        if (error_description) {
            std::cerr << "[Window] GLFW Error Description: " << error_description << std::endl;
        }
        
        // Platform-specific error analysis
        #ifdef _WIN32
        std::cerr << "[Window] Windows-specific diagnostics:" << std::endl;
        std::cerr << "[Window] - Check if running in Remote Desktop session" << std::endl;
        std::cerr << "[Window] - Verify graphics drivers are installed and up to date" << std::endl;
        std::cerr << "[Window] - Check if Windows Display Driver Model (WDDM) is available" << std::endl;
        #elif __linux__
        std::cerr << "[Window] Linux-specific diagnostics:" << std::endl;
        std::cerr << "[Window] - Check if DISPLAY environment variable is set" << std::endl;
        std::cerr << "[Window] - Verify X11 server is running (try 'echo $DISPLAY')" << std::endl;
        std::cerr << "[Window] - Check if Wayland compositor is available" << std::endl;
        std::cerr << "[Window] - Verify OpenGL libraries are installed" << std::endl;
        #elif __APPLE__
        std::cerr << "[Window] macOS-specific diagnostics:" << std::endl;
        std::cerr << "[Window] - Check if running in SSH session without X11 forwarding" << std::endl;
        std::cerr << "[Window] - Verify Quartz display services are available" << std::endl;
        #endif
        
        std::cerr << "[Window] RECOMMENDATION: Use --headless mode for environments without display" << std::endl;
        
        glfwTerminate();
        NeonGlyph::Logger::LogLine("Window Create failed: GLFW window creation failed");
        return Result::InitializationFailed;
    }
    
    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    NeonGlyph::Logger::LogEvent("Window", "Create", ms);
    std::cout << "[Window] GLFW window created successfully in " << ms << "ms" << std::endl;

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

    // Set up window iconify (minimize) callback
    glfwSetWindowIconifyCallback(m_window, [](GLFWwindow* window, int iconified) {
        auto appWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (appWindow) {
            appWindow->m_minimized = (iconified == GLFW_TRUE);
        }
    });

    // Set up window maximize callback to handle restoration
    glfwSetWindowMaximizeCallback(m_window, [](GLFWwindow* window, int maximized) {
        auto appWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (appWindow) {
            // When restored from minimized state, ensure we're not marked as minimized
            if (maximized == GLFW_FALSE) {
                // Check actual iconified state
                appWindow->m_minimized = (glfwGetWindowAttrib(window, GLFW_ICONIFIED) == GLFW_TRUE);
            }
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

bool Window::IsMinimized() const {
    if (!m_window) return false;
    return glfwGetWindowAttrib(m_window, GLFW_ICONIFIED) == GLFW_TRUE;
}

bool Window::IsVisible() const {
    if (!m_window) return false;
    return glfwGetWindowAttrib(m_window, GLFW_VISIBLE) == GLFW_TRUE && !IsMinimized();
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
