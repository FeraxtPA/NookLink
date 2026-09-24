
// Handles text rendering with various alignment and fitting modes.
// Provides methods for drawing text at different positions with proper sizing.


#pragma once

#include <raylib.h>
#include <string>
#include <vector>

class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();

   
    void DrawSimpleText(const std::string& text, Vector2 pos, float fontSize, Color col, float renderScale = 1.0f);

   
    void DrawTextCentered(const std::string& text, Vector2 centerPos, float fontSize, Color col, float renderScale = 1.0f);

  
    void DrawTextFitted(const std::string& text, Vector2 centerPos, float maxWidth, float fontSize, Color col, float renderScale = 1.0f);

  
    float Measure(const std::string& text, float fontSize, float renderScale = 1.0f) const;

private:
    // Support multiple pre-rasterized sizes to avoid upscaling the atlas.
    std::vector<Font> m_Fonts;
    std::vector<int> m_FontSizes;
    float m_Spacing = 1.0f;

    void InitFont();
    std::string FitTextToWidth(const std::string& text, float maxWidth, float fontSize, float renderScale);
    // Choose the best pre-rendered font for a requested font size.
    const Font* GetFontForSize(float fontSize) const;
};