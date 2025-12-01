#include "textRenderer.h"
#include "colors.h"

TextRenderer::TextRenderer() {
    
    InitFont();
    m_EllipsisWidth = MeasureTextEx(m_Font, "...", m_FontSize, m_Spacing).x;
}

TextRenderer::~TextRenderer() {
    UnloadFont(m_Font);
}

void TextRenderer::InitFont()
{

        int codepoints[101] = { 0 };
        for (int i = 0; i < 95; i++) codepoints[i] = 32 + i; // ASCII 32-126


        codepoints[95] = 0x00BC; // 1/4 (Vulgar Fraction One Quarter)
        codepoints[96] = 0x00BD; // 1/2 (Vulgar Fraction One Half)
        codepoints[97] = 0x00BE; // 3/4 (Vulgar Fraction Three Quarters)
        codepoints[98] = 0x2605; // star (Black Star)


        m_Font = LoadFontEx("assets/DejaVuSans.ttf", 48, codepoints, 99);

        SetTextureFilter(m_Font.texture, TEXTURE_FILTER_POINT);
    
}

std::string TextRenderer::fitTextToWidth(const std::string& text, float maxWidth) {
    if (MeasureTextEx(m_Font, text.c_str(), m_FontSize, m_Spacing).x <= maxWidth) {
        return text;
    }

    std::string result;
    int low = 0;
    int high = static_cast<int>(text.size());

    while (low <= high) {
        int mid = (low + high) / 2;
        std::string test = text.substr(0, mid) + "...";
        float width = MeasureTextEx(m_Font, test.c_str(), m_FontSize, m_Spacing).x;

        if (width <= maxWidth) {
            result = test;
            low = mid + 1;
        }
        else {
            high = mid - 1;
        }
    }

    return result;
}

void TextRenderer::drawTextCentered(const std::string& text, Vector2 pos, float radius, Color col = NookCol::TEXT_ONNODE)
{
    float maxTextWidth = radius * 1.95f;

    if (text != m_CachedTextInput || radius != m_CachedRadius) {
        m_CachedTextInput = text;
        m_CachedRadius = radius;

        m_CachedFittedText = fitTextToWidth(text, maxTextWidth);
        m_CachedSize = MeasureTextEx(m_Font, m_CachedFittedText.c_str(), m_FontSize, m_Spacing);
    }

    Vector2 textPos = {
        pos.x - m_CachedSize.x / 2.0f,
        pos.y - m_CachedSize.y / 2.0f
    };

    DrawTextEx(m_Font, m_CachedFittedText.c_str(), textPos, m_FontSize, m_Spacing, col);
}
