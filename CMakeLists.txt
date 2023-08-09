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

# Project options
option(WITHOUT_CONSOLE "Does not show the console when the program is opened" OFF)

# Test the operating system
if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

# Default cache
# TODO: support Vulkan and Metal
set(3D_BACKEND CACHE STRING OpenGL3)

if(WIN32 OR LINUX)
    set(AVAILABLE_3D_BACKENDS OpenGL3 Vulkan)
elseif(APPLE)
    set(AVAILABLE_3D_BACKENDS Metal OpenGL3 Vulkan)
endif()

message(${AVAILABLE_3D_BACKENDS})

# External librairies
add_subdirectory("external/glad")
add_subdirectory("external/glfw")
add_subdirectory("external/imgui")
add_subdirectory("external/nativefiledialog-extended")
add_subdirectory("external/stb_image")

# For GLFWwindowHandler
include_directories(src/glfw_handler)

# Files for the project
set(src_files
    "include/im_config.h"
    "external/imgui_bindings/imgui_impl_opengl3.cpp"
    "include/tempo.h"
    "external/imgui_bindings/imgui_impl_glfw.cpp"
    "src/utils.cpp"
    "src/glfw_handler/glfw_window_handler.cpp"
    "src/tempo.cpp"
    "src/events.cpp"
    "src/log.cpp"
    "src/jobscheduler.cpp"
    "src/keyboard_shortcuts.cpp"
)

add_library(Tempo STATIC ${src_files})

if(WITHOUT_CONSOLE)
    if(MSVC)
        set_target_properties(Tempo PROPERTIES
            LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:WINDOWS")
    endif()
endif()

if(WIN32)
    target_link_libraries(Tempo PRIVATE glad glfw Imgui imm32 nfd stb_image)
else()
    target_link_libraries(Tempo PRIVATE glad glfw Imgui nfd stb_image)
endif()

# Includes
target_include_directories(Tempo PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_include_directories(Tempo PUBLIC "${PROJECT_SOURCE_DIR}/external/glad/include")
target_include_directories(Tempo PUBLIC "${PROJECT_SOURCE_DIR}/external/glfw/include")
target_include_directories(Tempo PUBLIC "${PROJECT_SOURCE_DIR}/external/imgui/imgui")
target_include_directories(Tempo PUBLIC "${PROJECT_SOURCE_DIR}/external/imgui/imgui/misc/cpp")

# For GLFWwindowHandler
target_include_directories(Tempo PRIVATE "${PROJECT_SOURCE_DIR}/glfw_handler")
target_include_directories(Tempo PUBLIC "${PROJECT_SOURCE_DIR}/external/imgui_bindings")
target_include_directories(Tempo PUBLIC "${PROJECT_SOURCE_DIR}/external/nativefiledialog-extended/src/include")
target_include_directories(Tempo PUBLIC "${PROJECT_SOURCE_DIR}/external/stb_image")

# Set compiler options
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
endif()

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${source_list})

set_target_properties(glad glfw Imgui nfd PROPERTIES FOLDER InternalTargets)

get_directory_property(is_subproject PARENT_DIRECTORY)

# Only build the test suite and utilities if this is not a subproject
if(NOT is_subproject)
    add_subdirectory("examples/minimal")
    add_subdirectory("examples/feature")
endif()