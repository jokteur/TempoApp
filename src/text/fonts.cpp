#include "fonts.h"
#include "fonts_private.h"

namespace Tempo {
    std::optional<FontID> AddFontFromFileTTF(const std::string& filename, float size_pixels, ImFontConfig font_cfg, ImVector<ImWchar> glyph_ranges, bool no_dpi) {
        return FONTM.AddFontFromFileTTF(filename, size_pixels, font_cfg, glyph_ranges, no_dpi);
    }

    bool AddIconsToFont(FontID font_id, const std::string& filename, ImFontConfig font_cfg, ImVector<ImWchar> glyph_ranges) {
        return FONTM.AddIconsToFont(font_id, filename, font_cfg, glyph_ranges);
    }

    void RemoveFont(FontID font_id) {
        FONTM.RemoveFont(font_id);
    }

    void PushFont(FontID font_id, float scale) {
        FONTM.PushFont(font_id, scale);
    }

    void PopFont() {
        FONTM.PopFont();
    }

    SafeImFontPtr GetImFont(FontID font_id) {
        return FONTM.GetImFont(font_id);
    }
}