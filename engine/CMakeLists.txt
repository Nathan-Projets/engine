add_library(
    Engine

    core/application.cpp
    core/engine.cpp
    
    helpers/log.hpp
    
    input/glfw/glfw_input_backend.cpp
    
    platform/window_manager.cpp
    platform/window.cpp
    platform/glfw/glfw_platform_backend.cpp

    renderer/renderer.cpp
    renderer/renderer_backend.cpp
    renderer/gl/gl_renderer_backend.cpp
)

target_include_directories(Engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Engine PUBLIC glad glfw glm stb assimp simdjson)