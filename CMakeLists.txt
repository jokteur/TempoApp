cmake_minimum_required(VERSION 3.14)

# C++ compiler flags
set(CMAKE_CXX_STANDARD 17)

# Project name
project(Tempo)

# External librairies
add_subdirectory("external/glfw")
add_subdirectory("external/nativefiledialog-extended")