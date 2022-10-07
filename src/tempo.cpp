#include "tempo.h"
#include "internal.h"

#include <iostream>
#include <iterator>
#include <unordered_map>
#include <cmath>
#include <vector>
#include <chrono>

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

    struct FontInfo {
        std::map<float, SafeImFontPtr> multi_scale_font;
        float scaling = 0;
        // Font parameters for ImGui
        std::string filename;
        float size_pixels;
        bool no_dpi = false;
        ImFontConfig font_cfg;
        ImVector<ImWchar> glyph_ranges;
        std::vector<FontInfo> icons; // Can add multiple icons to a font
    };

    struct Fonts {
        int push_pop_counter = 0;
        bool reconstruct_fonts = true;
        std::set<int> ghost_pushes;
        int font_counter = 0;
        std::map<uint32_t, FontInfo> font_atlas;
    };

    Fonts s_fonts;

    std::optional<FontID> AddFontFromFileTTF(const std::string& filename, float size_pixels, ImFontConfig font_cfg, ImVector<ImWchar> glyph_ranges, bool no_dpi) {
        assert(app_state.app_initialized && "AddFontFromFileTTF cannot be called when the application has not been initialized yet.");

        // Assert also for when called in between loop

        FontInfo font;
        font.filename = filename;
        font.size_pixels = size_pixels;
        font.font_cfg = font_cfg;
        font.glyph_ranges = glyph_ranges;
        font.no_dpi = no_dpi;


        // TODO: check if file exists and can be loaded
        // auto& io = ImGui::GetIO();
        // if (glyph_ranges)

        // ImFont* imfont = nullptr;
        // if (font.glyph_ranges.empty())
        //     imfont = io.Fonts->AddFontFromFileTTF(font.filename.c_str(), size_pixels, &font.font_cfg);
        // else
        //     imfont = io.Fonts->AddFontFromFileTTF(font.filename.c_str(), size_pixels, &font.font_cfg, &font.glyph_ranges[0]);
        // if (imfont == nullptr) {
        //     return std::optional<FontID>();
        // }
        // font.multi_scale_font[1.f] = s_fonts.font_atlas.begin()->second.multi_scale_font.begin()->second;
        s_fonts.font_counter++;
        s_fonts.reconstruct_fonts = true;

        FontID font_id = (FontID)s_fonts.font_counter;
        s_fonts.font_atlas.insert(std::make_pair(font_id, font));

        return std::optional<FontID>(font_id);
    }

    bool AddIconsToFont(FontID font_id, const std::string& filename, ImFontConfig font_cfg, ImVector<ImWchar> glyph_ranges) {
        assert(app_state.app_initialized && "AddIconsToFont cannot be called when the application has not been initialized yet.");

        font_cfg.MergeMode = true;

        if (s_fonts.font_atlas.find(font_id) == s_fonts.font_atlas.end()) {
            return false;
        }
        FontInfo icon_font;
        icon_font.filename = filename;
        icon_font.font_cfg = font_cfg;
        icon_font.glyph_ranges = glyph_ranges;

        FontInfo& font_info = s_fonts.font_atlas[font_id];
        // auto& io = ImGui::GetIO();

        s_fonts.reconstruct_fonts = true;
        font_info.icons.push_back(icon_font);

        // PushFont(font_id);
        // io.Fonts->AddFontFromFileTTF(filename.c_str(), font_info.size_pixels, font_cfg, glyph_ranges);
        // PopFont();
        return true;
    }

    void RemoveFont(FontID font_id) {
        if (s_fonts.font_atlas.find(font_id) != s_fonts.font_atlas.end()) {
            // Invalidate all references to ImFont*
            for (auto pair : s_fonts.font_atlas[font_id].multi_scale_font) {
                pair.second->im_font = nullptr;
            }
            s_fonts.font_atlas.erase(font_id);
        }
    }

    void PushFont(FontID font_id, float scale) {
        assert(app_state.loop_running && "PushFont cannot be called outside of the main loop of the application");

        s_fonts.push_pop_counter++;
        FontInfo font_info = s_fonts.font_atlas[font_id];

        if (s_fonts.font_atlas.find(font_id) == s_fonts.font_atlas.end()
            || font_info.multi_scale_font.empty()) {
            s_fonts.ghost_pushes.insert(s_fonts.push_pop_counter);
            return;
        }

        // FIXME : multiple DPI support
        ImFont* font = (*(font_info.multi_scale_font.begin())).second->im_font;
        font->Scale = scale;
        ImGui::PushFont(font);
    }

    void PopFont() {
        assert(app_state.loop_running && "PushFont cannot be called outside of the main loop of the application");
        if (s_fonts.ghost_pushes.find(s_fonts.push_pop_counter) == s_fonts.ghost_pushes.end()) {
            ImGui::PopFont();
        }
        else {
            s_fonts.ghost_pushes.erase(s_fonts.push_pop_counter);
        }
        s_fonts.push_pop_counter--;
    }

    SafeImFontPtr GetImFont(FontID font_id) {
        if (font_id == -1) {
            return std::make_shared<SafeImFont>(SafeImFont{ nullptr });
        }
        FontInfo font_info = s_fonts.font_atlas[font_id];
        // TODO: multi scale atlas
        if (font_info.multi_scale_font.empty())
            return std::make_shared<SafeImFont>(SafeImFont{ nullptr });
        return font_info.multi_scale_font.begin()->second;
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

        //if (config.DPI_aware)
        //    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

        if (config.default_window_width == 0 || config.default_window_height == 0) {
            config.default_window_width = 800;
            config.default_window_height = 600;
        }

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
                s_fonts.reconstruct_fonts = true;
                app_state.global_scaling = global_xscale;
            }

            if (s_fonts.reconstruct_fonts) {
                io.Fonts->Clear();
                // For each font, we need one FontTexture per scale
                for (auto& font_pair : s_fonts.font_atlas) {
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
                        for (auto c : font.glyph_ranges) {
                            std::cout << c << std::endl;
                        }
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

            s_fonts.reconstruct_fonts = false;

            // ImGuiPlatformIO& platorm_io = ImGui::GetPlatformIO();

            app_state.before_frame = false;

            int width, height;
            glfwGetFramebufferSize(main_window, &width, &height);
            renderApplication(main_window, width, height, application);
            if (app_state.redraw) {
                app_state.redraw = false;
                glfwPostEmptyEvent();
            }

            JobScheduler::getInstance().finalizeJobs();

            if (glfwWindowShouldClose(main_window)) {
                scheduler.abortAll();
            }

        } while (!glfwWindowShouldClose(main_window) || scheduler.isBusy());

        app_state.loop_running = false;
        app_state.app_initialized = false;

        event_queue.unsubscribe(&tempo_listener);
        // Shut down ImGui and ImPlot
        ImGui_ImplOpenGL3_DestroyFontsTexture();
        ImGui_ImplGlfw_Shutdown();
        ImGui_ImplOpenGL3_Shutdown();
        application->AfterLoop();
        ImGui::DestroyContext();

        // Shut down native file dialog lib
        // NFD::Quit();

        // Shut down glfw
        glfwDestroyWindow(main_window);
        glfwTerminate();

        return 0;
    }
}