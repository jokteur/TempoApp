#pragma once

#include "im_config.h"
#include <imgui.h>
#ifdef ADVANCED_TEXT
#include "imgui_freetype.h"
#endif
#include <memory>
#include <string>
#include <optional>

namespace Tempo {
    /**
     * @brief ImFont* passed to the user could be dereferenced at any moment
     * This structure, along with a shared ptr can be used to determine if
     * ImFont* is still valid
     */
    struct SafeImFont {
        ImFont* im_font;
    };
    using SafeImFontPtr = std::shared_ptr<SafeImFont>;
    typedef int FontID;

    /**
     * @brief Adds a font (from file) that knows the DPI of the current viewport
     *
     * Using these fonts is similar to the ImGUI ImGui::PushFont and ImGui::PopFont,
     * instead, one must use the equivalent Tempo::PushFont and Tempo::PopFont
     *
     * By default, the first call to this function will be the default font file
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
     * @param no_dpi if true, is not influenced by dpi changes
     * @return std::optional<FontID> returns a FontID if it succeeded
     */
    std::optional<FontID> AddFontFromFileTTF(const std::string& filename, float size_pixels, ImFontConfig font_cfg = ImFontConfig{}, ImVector<ImWchar> glyph_ranges = ImVector<ImWchar>(), bool no_dpi = false);


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
    bool AddIconsToFont(FontID font_id, const std::string& filename, ImFontConfig font_cfg = ImFontConfig{}, ImVector<ImWchar> glyph_ranges = ImVector<ImWchar>());

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
    void PushFont(FontID font_id, float scale = 1.f);

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
     * @brief Returns the corresponding im font ptr from Tempo's font id
     *
     * @param font_id
     */
    SafeImFontPtr GetImFont(FontID font_id);
}