#include "textRenderer.h"
#include "colors.h"

TextRenderer::TextRenderer() {
    m_Font = LoadFontEx("Assets/OB.ttf", 32, nullptr, 0);
    SetTextureFilter(m_Font.texture, TEXTURE_FILTER_POINT);
    m_EllipsisWidth = MeasureTextEx(m_Font, "...", m_FontSize, m_Spacing).x;
}

TextRenderer::~TextRenderer() {
    UnloadFont(m_Font);
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

void TextRenderer::drawTextCentered(const std::string& text, Vector2 pos, float radius)
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

    DrawTextEx(m_Font, m_CachedFittedText.c_str(), textPos, m_FontSize, m_Spacing, NookCol::TEXT_ONNODE );
}
