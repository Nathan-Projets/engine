# Determine output directory
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin/Debug/assets)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin/Release/assets)
else()
    message(WARNING "Unknown build type: ${CMAKE_BUILD_TYPE}, using default output directory")
    set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin/Debug/assets)
endif()

# Collect all files recursively
file(GLOB_RECURSE RESOURCE_FILES RELATIVE "${CMAKE_CURRENT_LIST_DIR}" "*")

foreach(RESOURCE_FILE ${RESOURCE_FILES})
    if(RESOURCE_FILE STREQUAL "${CMAKE_CURRENT_LIST_FILE}")
        continue()
    endif()

    set(SRC_FILE "${CMAKE_CURRENT_LIST_DIR}/${RESOURCE_FILE}")
    set(DEST_FILE "${OUTPUT_DIR}/${RESOURCE_FILE}")

    if(IS_DIRECTORY "${SRC_FILE}")
        continue()
    endif()

    get_filename_component(DEST_DIR "${DEST_FILE}" DIRECTORY)
    file(MAKE_DIRECTORY "${DEST_DIR}")

    configure_file("${SRC_FILE}" "${DEST_FILE}" COPYONLY)
endforeach()
