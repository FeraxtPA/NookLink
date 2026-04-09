
// Implementation of the TextRenderer class.
// Handles font loading and text rendering with various fitting strategies.


#include "textRenderer.h"

#include "utf8_utils.h"

#include <vector>


TextRenderer::TextRenderer() {
    InitFont();
}

TextRenderer::~TextRenderer() {
    UnloadFont(m_Font);
}

void TextRenderer::InitFont()
{
    std::vector<int> codepoints;
    auto addRange = [&codepoints](int begin, int end) {
        for (int cp = begin; cp <= end; ++cp) {
            codepoints.push_back(cp);
        }
    };

    // Broad multilingual + symbols set (Latin, Greek, Cyrillic, punctuation, icons).
    addRange(0x0020, 0x007E); // Basic Latin
    addRange(0x00A0, 0x00FF); // Latin-1 Supplement
    addRange(0x0100, 0x017F); // Latin Extended-A
    addRange(0x0180, 0x024F); // Latin Extended-B
    addRange(0x1E00, 0x1EFF); // Latin Extended Additional
    addRange(0x0370, 0x03FF); // Greek and Coptic
    addRange(0x0400, 0x04FF); // Cyrillic
    addRange(0x2000, 0x206F); // General punctuation
    addRange(0x20A0, 0x20CF); // Currency symbols
    addRange(0x2190, 0x21FF); // Arrows
    addRange(0x2600, 0x26FF); // Misc symbols
    addRange(0x2700, 0x27BF); // Dingbats

    m_Font = LoadFontEx("assets/DejaVuSans.ttf", 64, codepoints.data(), (int)codepoints.size());
    if (m_Font.texture.id == 0) {
        m_Font = GetFontDefault();
    }
    SetTextureFilter(m_Font.texture, TEXTURE_FILTER_BILINEAR);
}

float TextRenderer::Measure(const std::string& text, float fontSize) const {
    return MeasureTextEx(m_Font, text.c_str(), fontSize, m_Spacing).x;
}

std::string TextRenderer::FitTextToWidth(const std::string& text, float maxWidth, float fontSize) {
    if (Measure(text, fontSize) <= maxWidth) {
        return text;
    }

    std::string result = text;
    std::string ellipsis = "...";
    float ellipsisW = Measure(ellipsis, fontSize);

    // Remove trailing codepoints, not bytes, to keep UTF-8 valid.
    while (!result.empty() && (Measure(result, fontSize) + ellipsisW) > maxWidth) {
        size_t cursor = result.size();
        Utf8::ErasePrevCodepoint(result, cursor);
    }

    return result + ellipsis;
}

void TextRenderer::DrawSimpleText(const std::string& text, Vector2 pos, float fontSize, Color col)
{
    DrawTextEx(m_Font, text.c_str(), pos, fontSize, m_Spacing, col);
}

void TextRenderer::DrawTextCentered(const std::string& text, Vector2 centerPos, float fontSize, Color col)
{
    Vector2 size = MeasureTextEx(m_Font, text.c_str(), fontSize, m_Spacing);
    Vector2 drawPos = {
        centerPos.x - size.x / 2.0f,
        centerPos.y - size.y / 2.0f
    };
    DrawTextEx(m_Font, text.c_str(), drawPos, fontSize, m_Spacing, col);
}

void TextRenderer::DrawTextFitted(const std::string& text, Vector2 centerPos, float maxWidth, float fontSize, Color col)
{
    std::string toDraw = FitTextToWidth(text, maxWidth, fontSize);
    DrawTextCentered(toDraw, centerPos, fontSize, col);
}