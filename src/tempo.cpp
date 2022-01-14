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

        // Monitors can be added, substracted, change their scaling
        // This is why we need to keep track if there is any change
        ImVector<float> monitors_scales;
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

    struct FontInfo {
        std::map<float, ImFont*> multi_scale_font;
        float scaling = 0;
        // Font parameters for ImGui
        std::string filename;
        float size_pixels;
        const ImFontConfig* font_cfg;
        const ImWchar* glyph_ranges;
    };

    struct Fonts {
        int push_pop_counter = 0;
        int font_counter = 0;
        std::map<uint32_t, FontInfo> font_atlas;
    };

    Fonts s_fonts;

    std::optional<FontID> AddFontFromFileTTF(const std::string& filename, float size_pixels, const ImFontConfig* font_cfg, const ImWchar* glyph_ranges) {
        assert(app_state.app_initialized && "AddFontFromFileTTF cannot be called when the application has not been initialized yet.");
        FontInfo font;
        font.filename = filename;
        font.size_pixels = size_pixels;
        font.font_cfg = font_cfg;
        font.glyph_ranges = glyph_ranges;

        s_fonts.font_counter++;

        // TODO: check if file exists and can be loaded
        auto& io = ImGui::GetIO();
        io.Fonts->AddFontFromFileTTF(filename.c_str(), size_pixels, font_cfg, glyph_ranges);

        FontID font_id = (FontID)s_fonts.font_counter;
        s_fonts.font_atlas.insert(std::make_pair(font_id, font));

        return std::optional<FontID>(font_id);
    }

    void RemoveFont(FontID font_id) {
        if (s_fonts.font_atlas.find(font_id) != s_fonts.font_atlas.end()) {
            s_fonts.font_atlas.erase(font_id);
        }
    }

    void PushFont(FontID font_id) {
        assert(app_state.loop_running && "PushFont cannot be called outside of the main loop of the application");
        if (s_fonts.font_atlas.find(font_id) == s_fonts.font_atlas.end()) {
            ImGui::PushFont(nullptr);
            return;
        }
        s_fonts.push_pop_counter++;
        // FIXME : multiple DPI support
        FontInfo font_info = s_fonts.font_atlas[font_id];
        ImFont* font = (*(font_info.multi_scale_font.begin())).second;
        ImGui::PushFont(font);
    }

    void PopFont() {
        assert(app_state.loop_running && "PushFont cannot be called outside of the main loop of the application");
        ImGui::PopFont();
        s_fonts.push_pop_counter--;
    }

    void Begin(const char* name, bool* p_open, ImGuiWindowFlags flags) {
        ImGui::Begin(name, p_open, flags);
    }
    void End() {
        ImGui::End();
    }

    int Run(App* application, Config config) {
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

        float previous_scale = 0;
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


            bool change_fonts = false;


            // FIXME: multi-DPI system
            // right now, we are use the main window content scale
            // the code below is a WIP for multi-DPI

            // Look for the scaling on each monitor
            // and change fonts if there is some change
            // int monitors_count = 0;
            // GLFWmonitor** glfw_monitors = glfwGetMonitors(&monitors_count);
            // if (app_state.monitors_scales.size() != monitors_count)
            //     change_fonts = true;

            // app_state.monitors_scales.resize(monitors_count);
            // for (int n = 0;n < monitors_count; n++) {
            //     float x_scale, y_scale;
            //     glfwGetMonitorContentScale(glfw_monitors[n], &x_scale, &y_scale);
            //     if (app_state.monitors_scales[n] != x_scale) {
            //         change_fonts = true;
            //     }
            //     app_state.monitors_scales[n] = x_scale;
            // }

            float xscale, yscale;
            glfwGetWindowContentScale(main_window, &xscale, &yscale);
            if (xscale != previous_scale) {
                change_fonts = true;
                previous_scale = xscale;
            }

            if (change_fonts) {
                io.Fonts->Clear();
                // For each font, we need one FontTexture per scale
                for (auto& font_pair : s_fonts.font_atlas) {
                    FontInfo& font = font_pair.second;
                    font.multi_scale_font.clear();
                    ImFont* imfont = io.Fonts->AddFontFromFileTTF(font.filename.c_str(), xscale * font.size_pixels, font.font_cfg, font.glyph_ranges);
                    font.multi_scale_font[xscale] = imfont;

                    // For multi-DPI
                    // for (auto& scale : app_state.monitors_scales) {
                    //     ImFont* imfont = io.Fonts->AddFontFromFileTTF(font.filename.c_str(), scale * font.size_pixels, font.font_cfg, font.glyph_ranges);
                    //     font.multi_scale_font[scale] = imfont;
                    // }
                }
                io.Fonts->Build();
                ImGui_ImplOpenGL3_DestroyFontsTexture();
                ImGui_ImplOpenGL3_CreateFontsTexture();
            }

            // ImGuiPlatformIO& platorm_io = ImGui::GetPlatformIO();


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