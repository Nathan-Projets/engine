add_library(
    Engine
    
    core/application.cpp
    core/scene.hpp
    core/scene_backpack.cpp
    core/scene_load_testing.cpp
    
    input/input_manager.cpp
    input/keyboard_state.cpp
    input/mouse_state.cpp
    
    ecs/entity.hpp
    ecs/component.hpp
    ecs/system.hpp
    ecs/world.hpp
    ecs/systems/transform_system.hpp
    ecs/systems/render_system.hpp
    
    game/game_world.cpp
    game/components/transform.hpp
    game/components/mesh_renderer.hpp
    game/components/light.hpp
    game/components/camera.hpp
    game/systems/rotating_light_system.hpp
    
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