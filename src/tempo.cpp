#include "tempo.h"
#include "internal.h"

#include <iostream>
#include <iterator>
#include <unordered_map>
#include <cmath>
#include <vector>
#include <chrono>
#include <toml.hpp>

namespace Tempo {
    inline float min(float a, float b) {
        return (a < b) ? a : b;
    }

    static void glfw_error_callback(int error, const char* description) {
        std::cerr << "Glfw Error: \n"
            << error << " " << description << std::endl;
    }

    App::App() {}

    AppState app_state;

    struct AppData {
        // Fonts
        float scaling = 1.0f;
        float font_size = 15.0f;
    };

    void SetMultiViewportsFocusBehavior(bool focus_all) {
        GLFWwindowHandler::focus_all = focus_all;
    }

    void Begin(const char* name, bool* p_open, ImGuiWindowFlags flags) {
        ImGui::Begin(name, p_open, flags);
    }
    void End() {
        ImGui::End();
    }

    void SetWaitTimeout(double timeout) {
        app_state.wait_timeout = timeout;
    }

    void PollUntil(long long milliseconds) {
        app_state.poll_until = std::chrono::steady_clock::now()
            + std::chrono::milliseconds(milliseconds);
    }

    void SkipFrame() {
        app_state.skip_frame = true;
    }

    void SetVSync(int interval) {
        glfwSwapInterval(interval);
    }

    float GetScaling() {
        return app_state.global_scaling;
    }

    void PushAnimation(const std::string& name, long long int duration) {
        if (app_state.animations.count(name))
            return;
        app_state.animations[name] = Animation{
            std::chrono::steady_clock::now(),
            duration
        };
    }

    float GetProgress(const std::string& name) {
        if (!app_state.animations.count(name)) {
            return 1.f;
        }
        auto now = std::chrono::steady_clock::now();
        return min(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now - app_state.animations[name].tp).count()
            / (float)app_state.animations[name].duration,
            1.f);
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
        app_state.glsl_version = "#version 150";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
        app_state.glsl_version = "#version 130";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
        //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

        GLFWwindowHandler::setAppName(config.app_name);

        // Window config file
        std::string config_path = nameToAppConfigFile(config.app_name);
        auto window_config = loadWindowConfig(config_path);

        if (config.default_window_width == 0 || config.default_window_height == 0) {
            config.default_window_width = 800;
            config.default_window_height = 600;
        }

        if (window_config.width != 0 && window_config.height != 0) {
            config.default_window_width = (uint16_t)window_config.width;
            config.default_window_height = (uint16_t)window_config.height;
        }
        else {
            if (config.DPI_aware)
                glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
        }
        // For now, we don't set the position, because it can lead to weird behavior
        // (e.g. the window is not visible on the screen, because it is on a monitor that is not connected)
        config.maximized = window_config.maximized;

        // Create main window with graphics context
        if (config.maximized)
            glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
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
        //glfwWindowHint(GLFW_SAMPLES, 4);
        //glEnable(GL_MULTISAMPLE);

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

        Listener tempo_listener;
        tempo_listener.filter = "Tempo/*";
        tempo_listener.callback = [=](Event_ptr event) {
            std::string name = event->getName();
            auto pos = name.find_first_of("/");
            if (pos + 1 < name.size()) {
                std::string type = name.substr(pos + 1);
                if (type == "redraw") {
                    app_state.redraw = true;
                }
                if (type == "quit") {
                    app_state.run_app = false;
                }
            }
            };
        event_queue.subscribe(&tempo_listener);

        /* ==== Other configs  ==== */
        GLFWwindowHandler::addWindow(main_window, 0, true);
        GLFWwindowHandler::focus_all = config.viewports_focus_all;
        GLFWwindowHandler::application = application;

        app_state.app_initialized = true;
        application->m_glfw_poll_or_wait = config.poll_or_wait;

        application->InitializationBeforeLoop();

        app_state.loop_running = true;
        app_state.wait_timeout = config.wait_timeout;

        app_state.global_scaling = 0;
        /* ==== Main loop  ==== */
        do {
            io = ImGui::GetIO();
            (void)io;

            auto now = std::chrono::steady_clock::now();

            if (application->m_glfw_poll_or_wait == Config::POLL)
                glfwPollEvents();
            else {
                if (std::chrono::duration_cast<std::chrono::milliseconds>(app_state.poll_until - now).count() > 0) {
                    glfwPollEvents();
                }
                else {
                    if (app_state.wait_timeout > 0.)
                        glfwWaitEventsTimeout(app_state.wait_timeout);
                    else
                        glfwWaitEvents();
                }
            }

            event_queue.pollEvents();
            KeyboardShortCut::dispatchShortcuts();

            // Animation update
            std::vector<std::string> to_remove;
            for (auto& pair : app_state.animations) {
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - pair.second.tp).count();
                if (duration > pair.second.duration)
                    to_remove.push_back(pair.first);
            }
            for (auto& str : to_remove) {
                app_state.animations.erase(str);
            }

            app_state.before_frame = true;

            application->BeforeFrameUpdate();

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

            float global_xscale, yscale;
            glfwGetWindowContentScale(main_window, &global_xscale, &yscale);
            if (global_xscale != app_state.global_scaling) {
                FONTM.reconstruct_fonts = true;
                app_state.global_scaling = global_xscale;
            }

            if (FONTM.reconstruct_fonts) {
                io.Fonts->Clear();
                // For each font, we need one FontTexture per scale
                for (auto& font_pair : FONTM.font_atlas) {
                    FontInfo& font = font_pair.second;
                    // Render all previous fonts null
                    for (auto pair : font.multi_scale_font) {
                        pair.second->im_font = nullptr;
                    }
                    font.multi_scale_font.clear();

                    float xscale = global_xscale;
                    if (font.no_dpi) {
                        xscale = 1.f;
                    }

                    float size = xscale * font.size_pixels;
                    ImFont* imfont;

                    if (font.glyph_ranges.empty())
                        imfont = io.Fonts->AddFontFromFileTTF(font.filename.c_str(), size, &font.font_cfg);
                    else {
                        imfont = io.Fonts->AddFontFromFileTTF(font.filename.c_str(), size, &font.font_cfg, &font.glyph_ranges[0]);
                    }

                    font.multi_scale_font[xscale] = std::make_shared<SafeImFont>(SafeImFont{ imfont });

                    for (auto& icon_font : font.icons) {
                        ImFontConfig cfg = icon_font.font_cfg;
                        cfg.GlyphOffset = ImVec2(xscale * cfg.GlyphOffset.x, xscale * cfg.GlyphOffset.y);
                        cfg.GlyphExtraSpacing = ImVec2(xscale * cfg.GlyphExtraSpacing.x, xscale * cfg.GlyphExtraSpacing.y);
                        cfg.GlyphMaxAdvanceX = xscale * cfg.GlyphMaxAdvanceX;
                        cfg.GlyphMinAdvanceX = xscale * cfg.GlyphMinAdvanceX;
                        if (icon_font.glyph_ranges.empty())
                            io.Fonts->AddFontFromFileTTF(
                                icon_font.filename.c_str(),
                                size, &cfg);
                        else
                            io.Fonts->AddFontFromFileTTF(
                                icon_font.filename.c_str(),
                                size, &cfg, &icon_font.glyph_ranges[0]);
                    }
#ifdef __APPLE__
                    io.FontGlobalScale = 1.f / xscale;
#endif

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

            FONTM.reconstruct_fonts = false;

            // ImGuiPlatformIO& platorm_io = ImGui::GetPlatformIO();

            app_state.before_frame = false;

            int width, height;
            glfwGetFramebufferSize(main_window, &width, &height);
            renderApplication(main_window, width, height, application);
            if (app_state.redraw) {
                app_state.redraw = false;
                glfwPostEmptyEvent();
            }

            scheduler.finalizeJobs();

            if (glfwWindowShouldClose(main_window) && !scheduler.isBusy()) {
                scheduler.abortAll();
                KeyboardShortCut::emptyKeyEventsQueue();
                app_state.run_app = false;
            }

        } while (app_state.run_app);

        scheduler.abortAll();
        KeyboardShortCut::emptyKeyEventsQueue();

        app_state.loop_running = false;
        app_state.app_initialized = false;

        // event_queue.unsubscribe(&tempo_listener);
        // Shut down ImGui and ImPlot
        ImGui_ImplOpenGL3_DestroyFontsTexture();
        ImGui_ImplGlfw_Shutdown();
        ImGui_ImplOpenGL3_Shutdown();
        application->AfterLoop();
        ImGui::DestroyContext();

        // Shut down native file dialog lib
        // NFD::Quit();
        scheduler.quit();

        // Shut down glfw
        glfwDestroyWindow(main_window);
        glfwTerminate();

        return 0;
    }
}