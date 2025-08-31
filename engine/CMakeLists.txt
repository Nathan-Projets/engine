add_library(
    Engine
    
    core/application.cpp
    core/scene.hpp
    core/scene_backpack.cpp
    core/scene_load_testing.cpp
    
    render/shader.cpp
    render/camera/camera.hpp
    render/camera/camera_perspective.cpp
    render/mesh.cpp
    render/model.cpp
    
    resources/fileloader.cpp
    resources/registry.hpp
    resources/stb_impl.cpp
    resources/loaders/all.hpp
    resources/loaders/iloader.hpp
    resources/loaders/loader.hpp
    resources/loaders/auto_loader.hpp
    resources/loaders/obj_loader.cpp
    resources/manager.hpp
    
    helpers/log.hpp
)

target_include_directories(Engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Engine PUBLIC glad glfw glm stb assimp)