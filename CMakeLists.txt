cmake_minimum_required(VERSION 3.25)

project(FallGuysClone)

# set(CMAKE_CXX_FLAGS_DEBUG "/O1")
set(CMAKE_CXX_STANDARD 23)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

find_package(OpenGL REQUIRED)
if (!OPENGL_FOUND)
    message("System does not have OpenGL installed.")
endif (!OPENGL_FOUND)

# Doxygen documentation target
find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(DOXYGEN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile")
    if(EXISTS ${DOXYGEN_FILE})
        add_custom_target(documentation
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_FILE}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Generating documentation with Doxygen..."
            VERBATIM
        )
        message(STATUS "Doxygen found. Use 'cmake --build . -t documentation' to generate docs.")
    else()
        message(WARNING "Doxyfile not found at ${DOXYGEN_FILE}")
    endif()
else()
    message(STATUS "Doxygen not found. Documentation generation will be skipped.")
endif()

add_subdirectory(assets)

add_subdirectory(external/glad)
add_subdirectory(external/glfw)
add_subdirectory(external/glm)
add_subdirectory(external/stb)
add_subdirectory(external/assimp)
add_subdirectory(external/simdjson)

add_subdirectory(engine)
add_subdirectory(game)

add_subdirectory(src)