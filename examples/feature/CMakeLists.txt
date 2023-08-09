# Set C++ 17 compiler flags
set(CMAKE_CXX_STANDARD 20)

# Set project name
project(feature)

# Add source files and dependencies to executable
set(
	source_list
	"main.cpp"
)
add_executable(${PROJECT_NAME} ${source_list})
target_link_libraries(${PROJECT_NAME} PRIVATE Tempo)
include_directories(../..external/imgui/imgui)

# Copy the data (such as fonts) in build directory
add_custom_command(TARGET feature PRE_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/data/ $<TARGET_FILE_DIR:feature>)

# Set compiler options
if(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
	target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
endif()