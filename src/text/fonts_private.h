#pragma once
#include "fonts.h"
#include <map>
#include <vector>
#include <set>
#include <string>
#include <optional>

namespace Tempo {
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

    struct FontManager {
        int push_pop_counter = 0;
        bool reconstruct_fonts = true;
        std::set<int> ghost_pushes;
        int font_counter = 0;
        std::map<uint32_t, FontInfo> font_atlas;

        /**
         * Copy constructors stay empty, because of the Singleton
         */
        FontManager(FontManager const&) = delete;
        void operator=(FontManager const&) = delete;

        /**
         * @return instance of the Singleton of the Job Scheduler
         */
        static FontManager& getInstance() {
            static FontManager instance;
            return instance;
        }

        void manage(float global_xscale);
        std::optional<FontID> AddFontFromFileTTF(const std::string& filename, float size_pixels, ImFontConfig font_cfg = ImFontConfig{}, ImVector<ImWchar> glyph_ranges = ImVector<ImWchar>(), bool no_dpi = false);
        bool AddIconsToFont(FontID font_id, const std::string& filename, ImFontConfig font_cfg = ImFontConfig{}, ImVector<ImWchar> glyph_ranges = ImVector<ImWchar>());
        void RemoveFont(FontID font_id);
        void PushFont(FontID font_id, float scale = 1.f);
        void PopFont();
        SafeImFontPtr GetImFont(FontID font_id);
    private:
        FontManager() = default;
    };

#define FONTM FontManager::getInstance()
}