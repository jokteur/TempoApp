# Require minimum standard version of CMake
cmake_minimum_required (VERSION 3.14)

# Set C++ 17 compiler flags
set(CMAKE_CXX_STANDARD 17)

# Set project name
project (Imgui LANGUAGES CXX)

# Set the source files to a list
set(
    source_list
	"imgui/imconfig.h"
    "imgui/imgui.cpp"
	"imgui/imgui.h"
	"imgui/imgui_demo.cpp"
	"imgui/imgui_draw.cpp"
	"imgui/imgui_internal.h"
    "imgui/imgui_tables.cpp"
	"imgui/imgui_widgets.cpp"
	"imgui/imstb_rectpack.h"
	"imgui/imstb_textedit.h"
	"imgui/imstb_truetype.h"
)	

# Create executable file as project name
add_library(Imgui ${source_list})
