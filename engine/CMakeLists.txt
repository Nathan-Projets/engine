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
    ecs/components/name.hpp
    ecs/components/mesh_renderer.hpp
    ecs/components/light.hpp
    ecs/components/camera.hpp
    
    render/camera/camera_projection.hpp
    render/camera/perspective_projection.cpp
    render/resource_gpu_uploader.cpp
    render/resource_gpu_uploader.hpp
    render/shader_domain.hpp
    render/uniform_buffer.cpp
    render/renderer.cpp

    resources/stb_impl.cpp

    resources/handle.hpp
    resources/resource.hpp
    resources/resource_loader.hpp
    resources/resource_manager.cpp
    resources/resource_pool.hpp
    resources/resource_state.hpp

    resources/units/material.hpp
    resources/units/mesh.hpp
    resources/units/shader.hpp
    resources/units/texture.hpp

    resources/loaders/scene_loader.cpp
    resources/loaders/material_loader.hpp
    resources/loaders/shader_loader.hpp
    resources/loaders/texture_loader.hpp

    ui/debug/debug_ui.cpp
    ui/debug/panels/debug_animation_panel.cpp
    ui/debug/panels/debug_loading_panel.cpp
    ui/debug/panels/debug_stats_panel.cpp
    ui/editor/panels/editor_files_panel.cpp
    ui/editor/panels/editor_inspector_panel.cpp
    ui/editor/panels/editor_outliner_panel.cpp
    ui/editor/panels/editor_viewport_panel.cpp
    
    helpers/log.hpp
)

if (MSVC)
    set_source_files_properties(
        resources/stb_impl.cpp
        PROPERTIES
            COMPILE_OPTIONS "$<$<CONFIG:Debug>:/O2>"
            VS_SETTINGS "$<$<CONFIG:Debug>:BasicRuntimeChecks=Default>"
    )
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    set_source_files_properties(
        resources/stb_impl.cpp
        PROPERTIES
            COMPILE_OPTIONS "$<$<CONFIG:Debug>:-O3>"
    )
endif ()

target_include_directories(Engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Engine PUBLIC glad glfw glm stb assimp simdjson imgui)