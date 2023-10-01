#include "../internal.h"
#include "fonts_private.h"

namespace Tempo {
    std::optional<FontID> FontManager::AddFontFromFileTTF(const std::string& filename, float size_pixels, ImFontConfig font_cfg, ImVector<ImWchar> glyph_ranges, bool no_dpi) {
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
        // font.multi_scale_font[1.f] = font_atlas.begin()->second.multi_scale_font.begin()->second;
        font_counter++;
        reconstruct_fonts = true;

        FontID font_id = (FontID)font_counter;
        font_atlas.insert(std::make_pair(font_id, font));

        return std::optional<FontID>(font_id);
    }
    bool FontManager::AddIconsToFont(FontID font_id, const std::string& filename, ImFontConfig font_cfg, ImVector<ImWchar> glyph_ranges) {
        // assert(app_state.app_initialized && "AddIconsToFont cannot be called when the application has not been initialized yet.");
        font_cfg.MergeMode = true;

        if (font_atlas.find(font_id) == font_atlas.end()) {
            return false;
        }
        FontInfo icon_font;
        icon_font.filename = filename;
        icon_font.font_cfg = font_cfg;
        icon_font.glyph_ranges = glyph_ranges;

        FontInfo& font_info = font_atlas[font_id];
        // auto& io = ImGui::GetIO();

        reconstruct_fonts = true;
        font_info.icons.push_back(icon_font);

        // PushFont(font_id);
        // io.Fonts->AddFontFromFileTTF(filename.c_str(), font_info.size_pixels, font_cfg, glyph_ranges);
        // PopFont();
        return true;
    }
    void FontManager::RemoveFont(FontID font_id) {
        if (font_atlas.find(font_id) != font_atlas.end()) {
            // Invalidate all references to ImFont*
            for (auto pair : font_atlas[font_id].multi_scale_font) {
                pair.second->im_font = nullptr;
            }
            font_atlas.erase(font_id);
        }
    }

    void FontManager::PushFont(FontID font_id, float scale) {
        // assert(app_state.loop_running && "PushFont cannot be called outside of the main loop of the application");
        push_pop_counter++;
        FontInfo font_info = font_atlas[font_id];

        if (font_atlas.find(font_id) == font_atlas.end()
            || font_info.multi_scale_font.empty()) {
            ghost_pushes.insert(push_pop_counter);
            return;
        }

        // FIXME : multiple DPI support
        ImFont* font = (*(font_info.multi_scale_font.begin())).second->im_font;
        font->Scale = scale;
        ImGui::PushFont(font);
    }

    void FontManager::PopFont() {
        // assert(app_state.loop_running && "PushFont cannot be called outside of the main loop of the application");
        if (ghost_pushes.find(push_pop_counter) == ghost_pushes.end()) {
            ImGui::PopFont();
        }
        else {
            ghost_pushes.erase(push_pop_counter);
        }
        push_pop_counter--;
    }

    SafeImFontPtr FontManager::GetImFont(FontID font_id) {
        if (font_id == -1) {
            return std::make_shared<SafeImFont>(SafeImFont{ nullptr });
        }
        FontInfo font_info = font_atlas[font_id];
        // TODO: multi scale atlas
        if (font_info.multi_scale_font.empty())
            return std::make_shared<SafeImFont>(SafeImFont{ nullptr });
        return font_info.multi_scale_font.begin()->second;
    }

    void FontManager::manage(float global_xscale) {
        auto& io = ImGui::GetIO();
        if (reconstruct_fonts) {
            io.Fonts->Clear();
            // For each font, we need one FontTexture per scale
            for (auto& font_pair : font_atlas) {
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

        reconstruct_fonts = false;
    }
}