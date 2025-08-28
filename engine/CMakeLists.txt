add_library(
    Engine
    
    core/application.cpp
    core/scene.hpp
    core/scene_backpack.cpp
    core/scene_load_testing.cpp
    
    render/shader.cpp
    render/texture.cpp
    render/camera/camera.hpp
    render/camera/camera_perspective.cpp
    render/mesh.cpp
    render/model.cpp

    resources/fileloader.cpp
    resources/registry.hpp
    resources/manager.hpp
)

target_include_directories(Engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Engine PUBLIC glad glfw glm stb assimp)