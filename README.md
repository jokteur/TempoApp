# TempoApp
Template for building desktop app in C++ with [Dear ImGui](https://github.com/ocornut/imgui) and [GLFW](https://github.com/glfw/glfw).

This library takes care of the boilerplate for building desktop application with Dear ImGui, which is a **bloat-free graphical user interface library for C++**.

## Features
- Multi-platform: Windows, Linux and MacOS (WIP)
- DPI aware (see `Tempo::PushFont` and `Tempo::PopFont`)
- Native file dialogs
- Thread safe event manager for passing messages between different parts of the application (see [src/events.h](src/events.h))
- Multi-threaded task scheduler for launching non-blocking tasks (see [src/jobscheduler.h](src/jobscheduler.h))


## Minimal example
Suppose that you have cloned this repository into a folder called `TempoApp` and you are using CMake for building the application.

**CMakeLists.txt**
```C++
# Set C++ 17 compiler flags
set(CMAKE_CXX_STANDARD 17)

# Set project name
project(minimal)

add_subdirectory(TempoApp)
include_directories(TempoApp/include)
include_directories(TempoApp/external/imgui/imgui)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE Tempo)
```

**main.cpp**
```C++
#include <iostream>
#include <tempo.h>

class MainApp : public Tempo::App {
public:
    virtual ~MainApp() {}

    void FrameUpdate() override {
        ImGui::Begin("My window");

        if (ImGui::Button("Click me")) {
            std::cout << "Hello world" << std::endl;
        }

        ImGui::End();
    }
    void BeforeFrameUpdate() override {}
};

int main() {
    Tempo::Config config{
        .app_name = "TestApp",
        .app_title = "Hello world",
    };

    MainApp* app = new MainApp();
    Tempo::Run(app, config);

    return 0;
}
```

## FAQ

- **Q: Why this project?**
- **A**: I maintain this repository for my own projects. Feel free to use or modify this project for your own.

- **Q: What compilers are supported?**
- **A**: For the moment gcc, Xcode and MSVC

- **Q: What is the minimal supported C++ standard?**
- **A**: C++17

- **Q: Is it possible to use this library with previous C++ standards ?**
- **A**: No, and I don't plan to support this in the future. 