add_library(
    Engine
    
    core/application.cpp
    core/base_scene.cpp
    core/scene.hpp
    
    input/input_manager.cpp
    input/keyboard_state.cpp
    input/mouse_state.cpp
    
    ecs/entity.hpp
    ecs/component.hpp
    ecs/system.hpp
    ecs/world.hpp
    
    ecs/systems/physics_system.hpp
    ecs/systems/render_system.hpp
    
    ecs/components/transform.hpp
    ecs/components/mesh_renderer.hpp
    ecs/components/light.hpp
    ecs/components/camera.hpp
    
    render/shader.cpp
    render/camera/camera_projection.hpp
    render/camera/perspective_projection.cpp
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
    resources/loaders/scene_loader.cpp
    resources/manager.hpp
    
    helpers/log.hpp
)

target_include_directories(Engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Engine PUBLIC glad glfw glm stb assimp simdjson)