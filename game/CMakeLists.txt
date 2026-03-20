add_library(
    Game
    
    scene/game_scene.cpp
    scene/game_scene.hpp
    components/component_registration.cpp
    components/component_registration.hpp
    components/light_orbit_controller.hpp
    components/orbit_camera_controller.hpp
    systems/light_orbit_controller_system.hpp
    systems/orbit_camera_controller_system.hpp
    systems/rotating_light_system.hpp
)

target_include_directories(Game PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Game PUBLIC Engine)