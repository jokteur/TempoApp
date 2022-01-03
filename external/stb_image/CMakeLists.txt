# Require minimum standard version of CMake
cmake_minimum_required (VERSION 3.14)

# Set project name
project(stb_image)

# Set C++ 17 compiler flags
set(CMAKE_CXX_STANDARD 17)

# Add source files and dependencies to library
set(
    source_list
	"stb_image.h"
	"stb_image_write.h"
)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE source_list)