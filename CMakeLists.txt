cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 23)

project(FallGuysClone)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

find_package(OpenGL REQUIRED)
if (!OPENGL_FOUND)
    message("System does not have OpenGL installed.")
endif (!OPENGL_FOUND)

add_subdirectory(assets)
add_subdirectory(engine)
add_subdirectory(src)