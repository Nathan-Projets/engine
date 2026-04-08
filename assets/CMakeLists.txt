# Determine output directories
set(ASSET_OUTPUT_DIRS)
if(CMAKE_CONFIGURATION_TYPES)
    foreach(CONFIGURATION ${CMAKE_CONFIGURATION_TYPES})
        list(APPEND ASSET_OUTPUT_DIRS "${CMAKE_BINARY_DIR}/bin/${CONFIGURATION}/assets")
    endforeach()
elseif(CMAKE_BUILD_TYPE)
    list(APPEND ASSET_OUTPUT_DIRS "${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}/assets")
else()
    list(APPEND ASSET_OUTPUT_DIRS "${CMAKE_BINARY_DIR}/bin/Debug/assets")
endif()

# Collect all files recursively
file(GLOB_RECURSE RESOURCE_FILES CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_LIST_DIR}" "*")

foreach(RESOURCE_FILE ${RESOURCE_FILES})
    if(RESOURCE_FILE STREQUAL "${CMAKE_CURRENT_LIST_FILE}")
        continue()
    endif()

    set(SRC_FILE "${CMAKE_CURRENT_LIST_DIR}/${RESOURCE_FILE}")

    if(IS_DIRECTORY "${SRC_FILE}")
        continue()
    endif()

    foreach(OUTPUT_DIR ${ASSET_OUTPUT_DIRS})
        set(DEST_FILE "${OUTPUT_DIR}/${RESOURCE_FILE}")
        get_filename_component(DEST_DIR "${DEST_FILE}" DIRECTORY)
        file(MAKE_DIRECTORY "${DEST_DIR}")
        configure_file("${SRC_FILE}" "${DEST_FILE}" COPYONLY)
    endforeach()
endforeach()
