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
	"imgui/misc/cpp/imgui_stdlib.h"
	"imgui/misc/cpp/imgui_stdlib.cpp"
	"imgui/imgui_internal.h"
    "imgui/imgui_tables.cpp"
	"imgui/imgui_widgets.cpp"
	"imgui/imstb_rectpack.h"
	"imgui/imstb_textedit.h"
	"imgui/imstb_truetype.h"
)	

# Create executable file as project name
if (ADVANCED_TEXT)
	add_library(Imgui ${source_list} "imgui/misc/freetype/imgui_freetype.cpp" "imgui/misc/freetype/imgui_freetype.h")
    target_link_libraries(Imgui PRIVATE ${FREETYPE_LIBRARIES} lunasvg)
    target_include_directories(Imgui PUBLIC ${FREETYPE_INCLUDE_DIRS})
    target_include_directories(Imgui PUBLIC ${LUNASVG_INCDIR})
else()
	add_library(Imgui ${source_list})
endif()
target_include_directories(Imgui PUBLIC "${PROJECT_SOURCE_DIR}/imgui")
