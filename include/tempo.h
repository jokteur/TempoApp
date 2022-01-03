#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>

#include <string>
#include <functional>
#include <optional>
#include <vector>
#include <cstdint>
#include <iostream>

//compatibility with older versions of Visual Studio
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

namespace Tempo {
    /**
     * @brief Configure the application before runtime
     *
     */
    struct Config {
        std::string app_name = "";

        // Main window settings
        std::string app_title = "";
        uint16_t default_window_height = 0;
        uint16_t default_window_width = 0;
        bool force_default_window_size = false;

        // ImGui configs flags
        int imgui_config_flags = ImGuiConfigFlags_ViewportsEnable | ImGuiConfigFlags_DockingEnable;

        // GLFW poll or wait
        enum GLFW_events { POLL, WAIT };
        GLFW_events poll_or_wait = WAIT;

        // DPI
        bool DPI_aware = true;

        // JobScheduler settings
        uint8_t worker_pool_size = 1;
    };

    /**
     * @brief Class for defining the application.
     *
     * One should inherit from this class to define the application
     * After that, the class should be given to Run() along with a Config object
     * to run the application
     */
    class App {
    private:
        Config::GLFW_events m_glfw_poll_or_wait;
    protected:
        GLFWwindow* m_main_window;
    public:
        App();
        virtual ~App() {}

        // Update tick
        virtual void FrameUpdate() {}
        virtual void BeforeFrameUpdate() {}

        // Returns the main GLFW window
        GLFWwindow* GetWindow() { return m_main_window; }

        friend int Run(App* application, Config config);
    };

    int Run(App* application, Config config);
}