#pragma once

#include <raylib.h>
#include <string>

class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();

    void drawTextCentered(const std::string& text, Vector2 pos, float radius);

   
private:
    Font m_Font;
    float m_FontSize = 32.0f;
    float m_Spacing = 1.0f;

    std::string m_CachedTextInput;
    std::string m_CachedFittedText;
    float m_CachedRadius = -1.0f;
    Vector2 m_CachedSize = { 0, 0 };
    float m_EllipsisWidth = 0.0f;

    std::string fitTextToWidth(const std::string& text, float maxWidth);
};
