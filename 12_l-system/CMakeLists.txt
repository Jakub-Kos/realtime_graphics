# Set the minimum CMake version
cmake_minimum_required(VERSION 3.10)

project(12_lsystem)

add_executable(12_lsystem 
	main.cpp
	l_system.hpp
	renderer.hpp
	scene_definition.hpp
	../utils/error_handling.hpp
	../utils/ogl_resource.hpp
	../utils/shader.hpp
	../utils/window.hpp
)
target_sources(12_lsystem PRIVATE 
	${CMAKE_CURRENT_SOURCE_DIR}/../glad/src/glad.c
)
target_link_libraries(12_lsystem utils glm::glm glfw OpenGL::GL)
target_include_directories(12_lsystem PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/../glad/include
	${CMAKE_CURRENT_SOURCE_DIR}/../utils
	${CMAKE_CURRENT_SOURCE_DIR}/..
	${CMAKE_CURRENT_SOURCE_DIR}
)

if(WIN32)
    # Windows: create NTFS junctions with mklink
    add_custom_target(link_resources12 ALL
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_BINARY_DIR}/12_l-system/shaders"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_BINARY_DIR}/12_l-system/rules"
        COMMAND cmd /c mklink /J "${CMAKE_BINARY_DIR}/12_l-system/shaders" "${CMAKE_SOURCE_DIR}/12_l-system/shaders"
        COMMAND cmd /c mklink /J "${CMAKE_BINARY_DIR}/12_l-system/rules"    "${CMAKE_SOURCE_DIR}/12_l-system/rules"
        COMMENT "Creating NTFS junctions for shaders and rules"
    )
    add_dependencies(12_lsystem link_resources12)
else()
    # Linux/macOS: use CMake to create symbolic links at configure time
    file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/00_HW-DoF/shaders" "${CMAKE_BINARY_DIR}/00_HW-DoF/data")
    file(CREATE_LINK
        "${CMAKE_SOURCE_DIR}/12_l-system/shaders"
        "${CMAKE_BINARY_DIR}/12_l-system/shaders"
        SYMBOLIC
    )
    file(CREATE_LINK
        "${CMAKE_SOURCE_DIR}/12_l-system/rules"
        "${CMAKE_BINARY_DIR}/12_l-system/rules"
        SYMBOLIC
    )
endif()