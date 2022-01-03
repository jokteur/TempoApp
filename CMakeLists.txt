cmake_minimum_required(VERSION 3.14)

# C++ compiler flags
set(CMAKE_CXX_STANDARD 17)

# Activate this property to "hide" targets (glad, glfw, Imgui, nfd, stb_image) to the end user
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Project name
project(Tempo)

# Remove glfw tests and examples
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)


# External librairies
add_subdirectory("external/glad")
add_subdirectory("external/glfw")
add_subdirectory("external/imgui")
add_subdirectory("external/nativefiledialog-extended")
add_subdirectory("external/stb_image")


# Files for the project
set(src_files
    "external/imgui_bindings/imgui_impl_glfw.h"
    "external/imgui_bindings/imgui_impl_glfw.cpp"
    "external/imgui_bindings/imgui_impl_opengl3.h"
    "external/imgui_bindings/imgui_impl_opengl3.cpp"
    "include/tempo.h"
    "src/tempo.cpp"
    "src/events.h"
    "src/events.cpp"
    "src/log.h"
    "src/log.cpp"
    "src/jobscheduler.h"
    "src/jobscheduler.cpp"
)

add_library(Tempo STATIC ${src_files})
target_link_libraries(Tempo PRIVATE glad glfw Imgui nfd stb_image)

# Includes
target_include_directories(Tempo PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_include_directories(Tempo PUBLIC "${PROJECT_SOURCE_DIR}/external/glad/include")
target_include_directories(Tempo PUBLIC "${PROJECT_SOURCE_DIR}/external/glfw/include")
target_include_directories(Tempo PUBLIC "${PROJECT_SOURCE_DIR}/external/imgui")
target_include_directories(Tempo PRIVATE "${PROJECT_SOURCE_DIR}/external/imgui_bindings")
target_include_directories(Tempo PRIVATE "${PROJECT_SOURCE_DIR}/external/nativefiledialog-extended/src/include")
target_include_directories(Tempo PRIVATE "${PROJECT_SOURCE_DIR}/external/stb_image")

# Set compiler options
if(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
	target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${source_list})

set_target_properties(glad glfw Imgui nfd PROPERTIES FOLDER InternalTargets)

get_directory_property(is_subproject PARENT_DIRECTORY)

# Only build the test suite and utilities if this is not a subproject
if(NOT is_subproject)
	add_subdirectory("examples/minimal")
endif()