#include "tempo.h"
#include "internal.h"

#include <iostream>

namespace Tempo {

    static void glfw_error_callback(int error, const char* description) {
        std::cerr << "Glfw Error: \n"
            << error << " " << description << std::endl;
    }

    App::App() {}

    struct AppState {
        bool error = false;
        std::string error_msg = "";
        const char* glsl_version;
    };

    int Run(App* application, Config config) {
        AppState app_state;

        /* ==== Initialize glfw  ==== */
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit()) {
            app_state.error = true;
            return 1;
        }

        // Decide GL+GLSL versions
#if __APPLE__
    // GL 3.2 + GLSL 150
        init_state.glsl_version = "#version 150";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
        app_state.glsl_version = "#version 130";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
        //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

        if (config.DPI_aware)
            glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

        if (config.default_window_width == 0 || config.default_window_height == 0) {
            config.default_window_width = 800;
            config.default_window_height = 600;
        }

        // Create main window with graphics context
        GLFWwindow* main_window = glfwCreateWindow(
            config.default_window_width,
            config.default_window_height,
            config.app_title.c_str(),
            nullptr,
            nullptr
        );

        glfwMakeContextCurrent(main_window);
        glfwSwapInterval(1);  // Enable vsync

        // Initialize OpenGL loader
        gladLoadGL();

        /* ==== Initialize ImGui  ==== */
        glfwWindowHint(GLFW_SAMPLES, 4);
        glEnable(GL_MULTISAMPLE);

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        io.ConfigFlags |= config.imgui_config_flags;

        // Setup Platform/Renderer bindings
        ImGui_ImplGlfw_InitForOpenGL(main_window, true);
        ImGui_ImplOpenGL3_Init(app_state.glsl_version);

        // Hack to make the ImGui windows look like normal windows
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }


        /* ==== Events & stuff  ==== */
        JobScheduler& scheduler = JobScheduler::getInstance();
        EventQueue& event_queue = EventQueue::getInstance();

        application->m_glfw_poll_or_wait = config.poll_or_wait;

        /* ==== Main loop  ==== */
        do {
            io = ImGui::GetIO();
            (void)io;

            if (application->m_glfw_poll_or_wait == Config::POLL)
                glfwPollEvents();
            else
                glfwWaitEvents();

            event_queue.pollEvents();

            application->BeforeFrameUpdate();

            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            application->FrameUpdate();

            // Rendering
            ImGui::Render();

            int display_w, display_h;
            glfwGetFramebufferSize(main_window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.5, 0.5, 0.5, 0);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                GLFWwindow* backup_current_context = glfwGetCurrentContext();
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
                glfwMakeContextCurrent(backup_current_context);
            }

            JobScheduler::getInstance().finalizeJobs();

            glfwSwapBuffers(main_window);

            if (glfwWindowShouldClose(main_window)) {
                scheduler.abortAll();
            }

        } while (!glfwWindowShouldClose(main_window) || scheduler.isBusy());

        // Shut down ImGui and ImPlot
        ImGui_ImplOpenGL3_DestroyFontsTexture();
        ImGui_ImplGlfw_Shutdown();
        ImGui_ImplOpenGL3_Shutdown();
        ImGui::DestroyContext();

        // Shut down native file dialog lib
        // NFD::Quit();

        // Shut down glfw
        glfwDestroyWindow(main_window);
        glfwTerminate();

        return 0;
    }
}