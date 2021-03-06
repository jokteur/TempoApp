#pragma once

#ifndef __gl_h_
#include <glad/glad.h>
#endif
#include <GLFW/glfw3.h>

#include <imgui.h>

#include <string>
#include <functional>
#include <optional>
#include <vector>
#include <cstdint>
#include <iostream>

#include "../src/jobscheduler.h"
#include "../src/events.h"

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
        bool no_console = false;
        bool maximized = false;

        // ImGui configs flags
        int imgui_config_flags = ImGuiConfigFlags_ViewportsEnable | ImGuiConfigFlags_DockingEnable;

        // GLFW poll or wait
        enum GLFW_events { POLL, WAIT };
        GLFW_events poll_or_wait = WAIT;
        double wait_timeout = 0.;

        // Multi viewports focus behavior (see SetMultiViewportsFocusBehavior for explanation)
        bool viewports_focus_all = true;

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

        /**
         * @brief Implement this function if you want to call some flags
         * before the loop of the application is launched, but after ImGui
         * has been initialized
         */
        virtual void InitializationBeforeLoop() {}

        /**
         * @brief Implement this function if you want to finish gracefully
         * some functions
         */
        virtual void AfterLoop() {}

        /**
         * @brief This is where you want to put all the ImGui calls to draw the UI
         * This function is called each loop continuisly
         */
        virtual void FrameUpdate() {}
        /**
         * @brief Sometimes, some
         *
         */
        virtual void BeforeFrameUpdate() {}

        // Returns the main GLFW window
        GLFWwindow* GetWindow() { return m_main_window; }

        friend int Run(App* application, Config config);
    };

    /**
     * @brief Set the Multi Viewports Focus Behavior
     *
     * @param focus_all if true, if the user clicks on any window of the app,
     * then all window are focused. This can be used when one wants that the
     * whole program is shown when another program hides one or more windows.
     */
    void SetMultiViewportsFocusBehavior(bool focus_all);

    typedef int FontID;

    /**
     * @brief Adds a font (from file) that knows the DPI of the current viewport
     *
     * Using these fonts is similar to the ImGUI ImGui::PushFont and ImGui::PopFont,
     * instead, one must use the equivalent Tempo::PushFont and Tempo::PopFont
     *
     * By default, the last call to this function will be the default font file
     * in the whole app.
     *
     * Must be called after the application has been initialized
     * It is recommended to use the function inside the MainApp::Initialization()
     * or MainApp::BeforeFrameUpdate()
     *
     * @param filename path to the TTF font
     * @param size_pixels relative pixel size of the font
     * @param font_cfg ImGUI font configuration flags
     * @param glyph_ranges ImGUI font ranges for glyphs
     * @return std::optional<FontID> returns a FontID if it succeeded
     */
    std::optional<FontID> AddFontFromFileTTF(const std::string& filename, float size_pixels, ImFontConfig font_cfg = ImFontConfig{}, std::vector<ImWchar> glyph_ranges = std::vector<ImWchar>());

    /**
     * @brief Adds a icon set to an existing font
     *
     * @param font fontID
     * @param filename path to the TTF font
     * @param font_cfg ImGUI font configuration flags
     * @param glyph_ranges ImGUI font ranges for glyphs
     * @return true if the font has been successfully added
     * @return false if the loading failed
     */
    bool AddIconsToFont(FontID font_id, const std::string& filename, ImFontConfig font_cfg = ImFontConfig{}, std::vector<ImWchar> glyph_ranges = std::vector<ImWchar>());

    /**
     * @brief Adds a font (from memory) that know the DPI of the current viewport
     *
     * Using these fonts is similar to the ImGUI ImGui::PushFont and ImGui::PopFont,
     * instead, one must use the equivalent PushDPIAwareFont and PopDPIAwareFont
     *
     * @param font_data array contained the TTF data
     * @param font_size size of the data array
     * @param size_pixels relative pixel size of the font
     * @param font_cfg ImGUI font configuration flags
     * @param glyph_ranges ImGUI font ranges for glyphs
     * @return std::optional<FontID> returns a FontID if it succeeded
     */
     // std::optional<FontID> AddFontFromMemoryTTF(void* font_data, int font_size, float size_pixels, const ImFontConfig* font_cfg = (const ImFontConfig*)0, const ImWchar* glyph_ranges = (const ImWchar*)0);

     /**
      * @brief Adds a font (from memory, compressed TTF) that know the DPI of the current viewport
      *
      * Using these fonts is similar to the ImGUI ImGui::PushFont and ImGui::PopFont,
      * instead, one must use the equivalent PushDPIAwareFont and PopDPIAwareFont
      *
      * @param compressed_font_data array contained the TTF data
      * @param compressed_font_size size of the data array
      * @param size_pixels relative pixel size of the font
      * @param font_cfg ImGUI font configuration flags
      * @param glyph_ranges ImGUI font ranges for glyphs
      * @return std::optional<FontID> returns a FontID if it succeeded
      */
      // std::optional<FontID> AddFontFromCompressedMemoryTTF(const void* compressed_font_data, int compressed_font_size, float size_pixels, const ImFontConfig* font_cfg = (const ImFontConfig*)0, const ImWchar* glyph_ranges = (const ImWchar*)0);

      /**
       * @brief Removes a DPI aware font from the atlas
       * If the FontID is not registered, this function does nothing
       *
       * @param font_id ID of the font, which should have been given by the AddDPIAwareFont* functions
       */
    void RemoveFont(FontID font_id);

    /**
     * @brief Pushes the DPI aware font to the front of the atlas
     *
     * If the FontID is not registered, it pushes the default ImGUI font
     * (and will not be DPI aware)
     *
     * This function should be used inside the main loop
     *
     * @param font_id
     */
    void PushFont(FontID font_id);

    /**
     * @brief Pops the last DPI aware pushed to the front of the atlas
     *
     * If no fonts are left to pop and this function is called, then
     * an assert is called
     *
     * This function should be used inside the main loop
     *
     */
    void PopFont();

    /**
     * @brief Replacement for ImGui::Begin to have multi-dpi awareness
     *
     * To be used with Tempo::End
     *
     * @param name
     * @param p_open
     * @param flags
     */
    void Begin(const char* name, bool* p_open = (bool*)0, ImGuiWindowFlags flags = 0);

    /**
     * @brief End of Tempo::Begin
     *
     */
    void End();

    /**
     * @brief If the application is set to Wait Events, then this will set a timeout
     * until the events are processed. If it is set to 0.0, then it is considered
     * that the application will wait until the next event.
     *
     * @param timeout timeout in seconds
     */
    void SetWaitTimeout(double timeout);

    /**
     * @brief If the application is set to Wait Events, then this will temporarily set the
     * application to Poll until the specified timer is expired
     *
     * @param seconds
     */
    void PollUntil(long long milliseconds);

    /**
     * @brief Activate or deactivate vsync during execution of the program
     *
     * @param vsync
     */
    void SetVSync(int interval);

    /**
     * @brief Returns the current DPI scaling of the main window
     *
     * @return float
     */
    float GetScaling();

    /**
     * @brief Pushes a new animation, which can be identified by name
     *
     * @param name of animation
     * @param duration in ms of the animation
     */
    void PushAnimation(const std::string& name, long long int duration);

    /**
     * @brief Get the animation progress
     *
     * @param name of animation
     * @return float progress (between 0 and 1) of the animation
     * if there is no animation in progress, it always returns 1.
     */
    float GetProgress(const std::string& name);

    int Run(App* application, Config config);
}