# Set C++ 17 compiler flags
set(CMAKE_CXX_STANDARD 20)

# Set project name
project(minimal)

# Add source files and dependencies to executable
set(
	source_list
	"main.cpp"
)
add_executable(${PROJECT_NAME} ${source_list})
target_link_libraries(${PROJECT_NAME} PRIVATE Tempo)

# Set compiler options
if(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
	target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
endif()