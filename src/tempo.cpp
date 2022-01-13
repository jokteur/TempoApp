#include "tempo.h"
#include "internal.h"

#include <iostream>
#include <iterator>

namespace Tempo {

    static void glfw_error_callback(int error, const char* description) {
        std::cerr << "Glfw Error: \n"
            << error << " " << description << std::endl;
    }

    App::App() {}

    struct AppState {
        bool error = false;
        bool loop_running = false;
        bool app_initialized = false;
        std::string error_msg = "";
        const char* glsl_version;
    };
    AppState app_state;

    struct AppData {
        // Fonts
        float scaling = 1.0f;
        float font_size = 15.0f;
    };

    void SetMultiViewportsFocusBehavior(bool focus_all) {
        GLFWwindowHandler::focus_all = focus_all;
    }

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

        /* ==== Other configs  ==== */
        GLFWwindowHandler::addWindow(main_window, 0, true);
        GLFWwindowHandler::focus_all = config.viewports_focus_all;
        GLFWwindowHandler::application = application;

        app_state.app_initialized = true;
        application->m_glfw_poll_or_wait = config.poll_or_wait;

        application->InitializationBeforeLoop();

        app_state.loop_running = true;

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

            // TODO: multi-scale system
            float xscale, yscale;
            bool change_fonts = false;
            GLFWmonitor* monitor = getCurrentMonitor(main_window);
            glfwGetMonitorContentScale(monitor, &xscale, &yscale);
            for (auto font_pair : s_fonts.font_atlas) {
                if (xscale != font_pair.second.scaling) {
                    change_fonts = true;
                    break;
                }
            }
            if (change_fonts) {
                std::cout << "Change scaling" << std::endl;
                io.Fonts->Clear();
                for (auto& font_pair : s_fonts.font_atlas) {
                    Font& font = font_pair.second;
                    if (xscale != font.scaling) {
                        ImFont* imfont = io.Fonts->AddFontFromFileTTF(font.filename.c_str(), xscale * font.size_pixels, font.font_cfg, font.glyph_ranges);
                        font.scaling = xscale;
                        font.imfont = imfont;
                    }
                }
                ImGui_ImplOpenGL3_DestroyFontsTexture();
                ImGui_ImplOpenGL3_CreateFontsTexture();
                io.Fonts->Build();
            }


            int width, height;
            glfwGetFramebufferSize(main_window, &width, &height);
            renderApplication(main_window, width, height, application);

            JobScheduler::getInstance().finalizeJobs();

            if (glfwWindowShouldClose(main_window)) {
                scheduler.abortAll();
            }

        } while (!glfwWindowShouldClose(main_window) || scheduler.isBusy());

        app_state.loop_running = false;
        app_state.app_initialized = false;
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