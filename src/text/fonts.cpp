#include "fonts.h"
#include "fonts_private.h"

namespace Tempo {
    std::optional<FontID> AddFontFromFileTTF(const std::string& filename, float size_pixels, ImFontConfig font_cfg, ImVector<ImWchar> glyph_ranges, bool no_dpi) {
        // assert(app_state.app_initialized && "AddFontFromFileTTF cannot be called when the application has not been initialized yet.");

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
        // font.multi_scale_font[1.f] = FONTM.font_atlas.begin()->second.multi_scale_font.begin()->second;
        FONTM.font_counter++;
        FONTM.reconstruct_fonts = true;

        FontID font_id = (FontID)FONTM.font_counter;
        FONTM.font_atlas.insert(std::make_pair(font_id, font));

        return std::optional<FontID>(font_id);
    }

    bool AddIconsToFont(FontID font_id, const std::string& filename, ImFontConfig font_cfg, ImVector<ImWchar> glyph_ranges) {
        // assert(app_state.app_initialized && "AddIconsToFont cannot be called when the application has not been initialized yet.");

        font_cfg.MergeMode = true;

        if (FONTM.font_atlas.find(font_id) == FONTM.font_atlas.end()) {
            return false;
        }
        FontInfo icon_font;
        icon_font.filename = filename;
        icon_font.font_cfg = font_cfg;
        icon_font.glyph_ranges = glyph_ranges;

        FontInfo& font_info = FONTM.font_atlas[font_id];
        // auto& io = ImGui::GetIO();

        FONTM.reconstruct_fonts = true;
        font_info.icons.push_back(icon_font);

        // PushFont(font_id);
        // io.Fonts->AddFontFromFileTTF(filename.c_str(), font_info.size_pixels, font_cfg, glyph_ranges);
        // PopFont();
        return true;
    }

    void RemoveFont(FontID font_id) {
        if (FONTM.font_atlas.find(font_id) != FONTM.font_atlas.end()) {
            // Invalidate all references to ImFont*
            for (auto pair : FONTM.font_atlas[font_id].multi_scale_font) {
                pair.second->im_font = nullptr;
            }
            FONTM.font_atlas.erase(font_id);
        }
    }

    void PushFont(FontID font_id, float scale) {
        // assert(app_state.loop_running && "PushFont cannot be called outside of the main loop of the application");

        FONTM.push_pop_counter++;
        FontInfo font_info = FONTM.font_atlas[font_id];

        if (FONTM.font_atlas.find(font_id) == FONTM.font_atlas.end()
            || font_info.multi_scale_font.empty()) {
            FONTM.ghost_pushes.insert(FONTM.push_pop_counter);
            return;
        }

        // FIXME : multiple DPI support
        ImFont* font = (*(font_info.multi_scale_font.begin())).second->im_font;
        font->Scale = scale;
        ImGui::PushFont(font);
    }

    void PopFont() {
        // assert(app_state.loop_running && "PushFont cannot be called outside of the main loop of the application");
        if (FONTM.ghost_pushes.find(FONTM.push_pop_counter) == FONTM.ghost_pushes.end()) {
            ImGui::PopFont();
        }
        else {
            FONTM.ghost_pushes.erase(FONTM.push_pop_counter);
        }
        FONTM.push_pop_counter--;
    }

    SafeImFontPtr GetImFont(FontID font_id) {
        if (font_id == -1) {
            return std::make_shared<SafeImFont>(SafeImFont{ nullptr });
        }
        FontInfo font_info = FONTM.font_atlas[font_id];
        // TODO: multi scale atlas
        if (font_info.multi_scale_font.empty())
            return std::make_shared<SafeImFont>(SafeImFont{ nullptr });
        return font_info.multi_scale_font.begin()->second;
    }
}