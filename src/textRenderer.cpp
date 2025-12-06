#include "textRenderer.h"
#include "colors.h"
#include <algorithm>

TextRenderer::TextRenderer() {
    InitFont();
}

TextRenderer::~TextRenderer() {
    UnloadFont(m_Font);
}

void TextRenderer::InitFont()
{
    // Load custom codepoints (fractions, stars)
    int codepoints[101] = { 0 };
    for (int i = 0; i < 95; i++) codepoints[i] = 32 + i; // ASCII 32-126

    codepoints[95] = 0x00BC; // 1/4
    codepoints[96] = 0x00BD; // 1/2
    codepoints[97] = 0x00BE; // 3/4
    codepoints[98] = 0x2605; // star

    
    m_Font = LoadFontEx("Assets/DejaVuSans.ttf", 32, codepoints, 99);
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

    // Binary search roughly or linear scan to cut text
    while (result.length() > 0 && (Measure(result, fontSize) + ellipsisW) > maxWidth) {
        result.pop_back();
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