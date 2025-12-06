#pragma once

#include <raylib.h>
#include <string>

class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();

    // Standard draw text
    void DrawSimpleText(const std::string& text, Vector2 pos, float fontSize, Color col);

    // Draws text centered at specific position
    void DrawTextCentered(const std::string& text, Vector2 centerPos, float fontSize, Color col);

    // Fits text inside a width, adding "..." if needed, and centers it
    void DrawTextFitted(const std::string& text, Vector2 centerPos, float maxWidth, float fontSize, Color col);

    // Helper to get text width
    float Measure(const std::string& text, float fontSize) const;

private:
    Font m_Font;
    float m_Spacing = 1.0f;

    void InitFont();
    std::string FitTextToWidth(const std::string& text, float maxWidth, float fontSize);
};