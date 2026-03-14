add_library(
    Game
    
    game_world.cpp
    game_world.hpp
    systems/rotating_light_system.hpp
)

target_include_directories(Game PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Game PUBLIC Engine)