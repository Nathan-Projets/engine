add_library(
    Engine

    core/application.cpp
    core/engine.cpp
    
    helpers/log.hpp
    
    input/input_manager.cpp
    input/keyboard_state.cpp
    input/mouse_state.cpp
    
    platform/window_manager.cpp
    platform/window.cpp
    platform/glfw/glfw_platform_backend.cpp
)

target_include_directories(Engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Engine PUBLIC glad glfw glm stb assimp simdjson)