add_library(
    Engine
    
    input/input_manager.cpp
    input/keyboard_state.cpp
    input/mouse_state.cpp
    
    helpers/log.hpp
)

target_include_directories(Engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Engine PUBLIC glad glfw glm stb assimp simdjson)