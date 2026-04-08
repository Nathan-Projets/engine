add_executable(
    Editor
    editor_main.cpp
)

add_executable(
    Game
    game_main.cpp
)

add_executable(
    ${PROJECT_NAME}
    editor_main.cpp
)

target_include_directories(Editor PRIVATE ${CMAKE_SOURCE_DIR})
target_include_directories(Game PRIVATE ${CMAKE_SOURCE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(Editor PRIVATE Engine GameRuntime simdjson)
target_link_libraries(Game PRIVATE Engine GameRuntime simdjson)
target_link_libraries(${PROJECT_NAME} PRIVATE Engine GameRuntime simdjson)