add_library(
    GameRuntime
    
    scene/game_scene.cpp
    scene/game_scene.hpp
    scene/editor_scene.hpp
    scene/runtime_scene_support.hpp
    scene/simple_game_scene.cpp
    scene/simple_game_scene.hpp
    components/component_registration.cpp
    components/component_registration.hpp
    components/light_orbit_controller.hpp
    components/orbit_camera_controller.hpp
    components/physics_listener.hpp
    components/player_controller.hpp
    systems/light_orbit_controller_system.hpp
    systems/orbit_camera_controller_system.hpp
    systems/physics_event_listener_system.hpp
    systems/physics_listener_demo_system.hpp
    systems/player_controller_system.hpp
    systems/rotating_light_system.hpp
)

target_include_directories(GameRuntime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(GameRuntime PUBLIC Engine)