cmake_minimum_required(VERSION 3.20)
project(NeonGlyph VERSION 3.0.1 LANGUAGES CXX)

enable_testing()
option(NEONGLYPH_BUILD_TESTS "Build test executables" OFF)

# C++20 Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build configuration
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type")
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")

# Compiler flags
if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus)
    add_compile_options(/MP) # Multi-processor compilation
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    add_compile_options(-march=native -mtune=native)
endif()

# Find packages
find_package(Vulkan REQUIRED)
find_package(Threads REQUIRED)
find_package(Freetype QUIET)
find_package(glfw3 QUIET)

# Ensure GLFW is available; if not, fetch and build it
include(FetchContent)
if(NOT glfw3_FOUND)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.9
  )
  FetchContent_MakeAvailable(glfw)
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Vulkan_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}/_deps/glfw-src/include
)

# Source files
file(GLOB_RECURSE SOURCES
    src/*.cpp
    src/*.h
    include/*.h
)
list(FILTER SOURCES EXCLUDE REGEX ".*/src/chaos/.*")
list(FILTER SOURCES EXCLUDE REGEX ".*/src/VulkanOptimizedContext.cpp$")
list(FILTER SOURCES EXCLUDE REGEX ".*/src/AIConductor.cpp$")
list(FILTER SOURCES EXCLUDE REGEX ".*/src/DirectorClientTcp.cpp$")

# Add Windows integration source files
option(WINDOWS_INTEGRATION "Enable Windows integration components" OFF)
if(WINDOWS_INTEGRATION)
  list(APPEND SOURCES 
      ${CMAKE_CURRENT_SOURCE_DIR}/src/WindowsIntegration.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/src/NeonGlyphApp.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/src/SeamlessWindow.cpp
  )
else()
  list(REMOVE_ITEM SOURCES 
      ${CMAKE_CURRENT_SOURCE_DIR}/src/WindowsIntegration.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/src/NeonGlyphApp.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/src/SeamlessWindow.cpp
  )
endif()

# Exclude story_demo from main NeonGlyph executable to avoid multiple entry points
list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/story_demo.cpp)
list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/production/PerformanceManagementSystem.cpp)

# Create executable
add_executable(NeonGlyph ${SOURCES})

option(BUILD_STORY_DEMO "Build optional StoryDemo executable" OFF)
if(BUILD_STORY_DEMO)
  file(GLOB_RECURSE STORY_DEMO_SOURCES
      src/*.cpp
      src/*.h
      include/*.h
  )
  list(REMOVE_ITEM STORY_DEMO_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
  list(REMOVE_ITEM STORY_DEMO_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/WindowsIntegration.cpp)
  list(REMOVE_ITEM STORY_DEMO_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/NeonGlyphApp.cpp)
  list(REMOVE_ITEM STORY_DEMO_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/SeamlessWindow.cpp)
  add_executable(StoryDemo src/story_demo.cpp ${STORY_DEMO_SOURCES})
  target_link_libraries(StoryDemo
      ${Vulkan_LIBRARIES}
      Threads::Threads
      winmm.lib
      mmdevapi.lib
      avrt.lib
      propsys.lib
      d3d11.lib
      dxgi.lib
      d3dcompiler.lib
      nlohmann_json::nlohmann_json
      glfw
  )
  target_include_directories(StoryDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_BINARY_DIR}/_deps/nlohmann_json-src/include)
  target_compile_definitions(StoryDemo PRIVATE VK_USE_PLATFORM_WIN32_KHR NEONGLYPH_HAVE_GLFW=1)
endif()

# Link libraries
target_link_libraries(NeonGlyph
    ${Vulkan_LIBRARIES}
    Threads::Threads
    # Windows-specific libraries
    winmm.lib
    mmdevapi.lib
    avrt.lib  # For audio thread priority functions
    # audioclient.lib  # This library doesn't exist - WASAPI is included in mmdevapi.lib
    propsys.lib
    d3d11.lib
    dxgi.lib
    d3dcompiler.lib
    # Explicit Vulkan library path
    "C:/VulkanSDK/1.4.328.1/Lib/vulkan-1.lib"
)

# Conditional linking
target_link_libraries(NeonGlyph glfw)
target_compile_definitions(NeonGlyph PRIVATE NEONGLYPH_HAVE_GLFW=0)
target_compile_definitions(NeonGlyph PRIVATE NEONGLYPH_HAVE_DIRECTOR=0)
target_include_directories(NeonGlyph PRIVATE ${CMAKE_BINARY_DIR}/_deps/glfw-src/include)
if(glfw_SOURCE_DIR)
  target_include_directories(NeonGlyph PRIVATE ${glfw_SOURCE_DIR}/include)
endif()
include(FetchContent)
FetchContent_Declare(nlohmann_json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
FetchContent_MakeAvailable(nlohmann_json)

# Preprocessor definitions
target_compile_definitions(NeonGlyph PRIVATE
    VK_USE_PLATFORM_WIN32_KHR
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    _WIN32_WINNT=0x0A00 # Windows 10
)

# Windows-specific libraries for system tray and integration
target_link_libraries(NeonGlyph shell32.lib user32.lib)
target_link_libraries(NeonGlyph nlohmann_json::nlohmann_json)
target_include_directories(NeonGlyph PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_BINARY_DIR}/_deps/nlohmann_json-src/include)

# Audio capture test executable
if(NEONGLYPH_BUILD_TESTS)
  add_executable(e2e_test tests/e2e_test.cpp)
  target_link_libraries(e2e_test nlohmann_json::nlohmann_json)
  target_include_directories(e2e_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_BINARY_DIR}/_deps/nlohmann_json-src/include)

  add_executable(AudioTest audio_test.cpp src/AudioEngine.cpp src/ConfigManager.cpp)
  target_include_directories(AudioTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_BINARY_DIR}/_deps/nlohmann_json-src/include)
  target_link_libraries(AudioTest
      Threads::Threads
      winmm.lib
      mmdevapi.lib
      avrt.lib
      nlohmann_json::nlohmann_json
  )
  target_compile_definitions(AudioTest PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00 _CRT_SECURE_NO_WARNINGS)
endif()

if(NEONGLYPH_BUILD_TESTS)
  add_executable(runtime_logic_tests
      tests/runtime_logic_tests.cpp
      src/ConfigManager.cpp
      src/VulkanContext.cpp
  )
  target_include_directories(runtime_logic_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(runtime_logic_tests ${Vulkan_LIBRARIES} Threads::Threads "C:/VulkanSDK/1.4.328.1/Lib/vulkan-1.lib")
  target_compile_definitions(runtime_logic_tests PRIVATE VK_USE_PLATFORM_WIN32_KHR NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

if(NEONGLYPH_BUILD_TESTS)
  add_executable(ascii_font_atlas_test
      tests/ascii_font_atlas_test.cpp
      src/VulkanContext.cpp
      src/ASCIIConverter.cpp
      src/ComputePipelines.cpp
  )
  target_include_directories(ascii_font_atlas_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(ascii_font_atlas_test ${Vulkan_LIBRARIES} Threads::Threads "C:/VulkanSDK/1.4.328.1/Lib/vulkan-1.lib")
  target_compile_definitions(ascii_font_atlas_test PRIVATE VK_USE_PLATFORM_WIN32_KHR NOMINMAX WIN32_LEAN_AND_MEAN NEONGLYPH_HAVE_FREETYPE=0)
endif()

if(NEONGLYPH_BUILD_TESTS)
  add_executable(gpu_pipeline_test
      tests/gpu_pipeline_test.cpp
      src/VulkanContext.cpp
      src/ASCIIConverter.cpp
      src/ComputePipelines.cpp
      src/ConfigManager.cpp
  )
  target_include_directories(gpu_pipeline_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(gpu_pipeline_test ${Vulkan_LIBRARIES} Threads::Threads "C:/VulkanSDK/1.4.328.1/Lib/vulkan-1.lib")
  target_compile_definitions(gpu_pipeline_test PRIVATE VK_USE_PLATFORM_WIN32_KHR NOMINMAX WIN32_LEAN_AND_MEAN NEONGLYPH_HAVE_FREETYPE=0)
endif()

# Asset copying
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})

# Install rules
install(TARGETS NeonGlyph DESTINATION bin)
install(DIRECTORY assets/ DESTINATION bin/assets)

# Optional ONNX Runtime integration
set(ONNXRUNTIME_HINT "$ENV{ONNXRUNTIME_DIR}")
find_path(ONNX_INCLUDE_DIR onnxruntime_cxx_api.h
  PATHS
    ${ONNXRUNTIME_HINT}/include
    "C:/Program Files/onnxruntime/include"
    "C:/Program Files (x86)/onnxruntime/include"
)
find_library(ONNX_RUNTIME_LIB NAMES onnxruntime
  PATHS
    ${ONNXRUNTIME_HINT}/lib
    ${ONNXRUNTIME_HINT}/lib/Release
    "C:/Program Files/onnxruntime/lib"
    "C:/Program Files (x86)/onnxruntime/lib"
)
if(ONNX_INCLUDE_DIR AND ONNX_RUNTIME_LIB)
  target_include_directories(NeonGlyph PRIVATE ${ONNX_INCLUDE_DIR})
  target_link_libraries(NeonGlyph ${ONNX_RUNTIME_LIB})
  target_compile_definitions(NeonGlyph PRIVATE NEONGLYPH_HAVE_ONNXRUNTIME=1)
else()
  message(WARNING "ONNX Runtime not found; AIConductor will be disabled")
  target_compile_definitions(NeonGlyph PRIVATE NEONGLYPH_HAVE_ONNXRUNTIME=0)
endif()

# CPack configuration
set(CPACK_PACKAGE_NAME "NeonGlyph")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VENDOR "NeonGlyph Studios")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "AI-Driven ASCII Visual Synthesis Engine")
include(CPack)
if(Freetype_FOUND)
  target_link_libraries(NeonGlyph Freetype::Freetype)
  target_compile_definitions(NeonGlyph PRIVATE NEONGLYPH_HAVE_FREETYPE=1)
else()
  message(WARNING "FreeType not found; font atlas generation will be disabled")
  target_compile_definitions(NeonGlyph PRIVATE NEONGLYPH_HAVE_FREETYPE=0)
endif()
if(NEONGLYPH_BUILD_TESTS)
  add_executable(config_validation_tests tests/config_validation_tests.cpp src/ConfigManager.cpp)
  target_include_directories(config_validation_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()

if(NEONGLYPH_BUILD_TESTS)
  add_executable(thread_lifecycle_tests tests/thread_lifecycle_tests.cpp src/chaos/FailureModeAnalysis.cpp)
  target_include_directories(thread_lifecycle_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(thread_lifecycle_tests Threads::Threads)
endif()

if(NEONGLYPH_BUILD_TESTS)
  add_executable(vulkan_resource_stress_test tests/vulkan_resource_stress_test.cpp src/VulkanContext.cpp)
  target_include_directories(vulkan_resource_stress_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(vulkan_resource_stress_test ${Vulkan_LIBRARIES} Threads::Threads "C:/VulkanSDK/1.4.328.1/Lib/vulkan-1.lib")
endif()
