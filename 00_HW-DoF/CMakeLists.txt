# Set the minimum CMake version
cmake_minimum_required(VERSION 3.10)

# Project definition
project(00_HW-DoF)

# Define the executable and its sources
add_executable(00_HW-DoF
    main.cpp
    renderer.hpp
    scene_definition.hpp
)

# Add glad implementation
target_sources(00_HW-DoF PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../glad/src/glad.c
)

# Link libraries
target_link_libraries(00_HW-DoF
    utils
    glm::glm
    glfw
    OpenGL::GL
)

# Include directories
target_include_directories(00_HW-DoF PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../glad/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../utils
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# --- Resource directory linking/junctions ---
if(WIN32)
    # Windows: create NTFS junctions with mklink
    add_custom_target(link_resources ALL
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_BINARY_DIR}/00_HW-DoF/shaders"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_BINARY_DIR}/00_HW-DoF/data"
        COMMAND cmd /c mklink /J "${CMAKE_BINARY_DIR}/00_HW-DoF/shaders" "${CMAKE_SOURCE_DIR}/00_HW-DoF/shaders"
        COMMAND cmd /c mklink /J "${CMAKE_BINARY_DIR}/00_HW-DoF/data"    "${CMAKE_SOURCE_DIR}/data"
        COMMENT "Creating NTFS junctions for shaders and data"
    )
    add_dependencies(00_HW-DoF link_resources)
else()
    # Linux/macOS: use CMake to create symbolic links at configure time
    file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/00_HW-DoF/shaders" "${CMAKE_BINARY_DIR}/00_HW-DoF/data")
    file(CREATE_LINK
        "${CMAKE_SOURCE_DIR}/00_HW-DoF/shaders"
        "${CMAKE_BINARY_DIR}/00_HW-DoF/shaders"
        SYMBOLIC
    )
    file(CREATE_LINK
        "${CMAKE_SOURCE_DIR}/data"
        "${CMAKE_BINARY_DIR}/00_HW-DoF/data"
        SYMBOLIC
    )
endif()