#pragma once

#include <raylib.h>
#include <string>

class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();

   
    void DrawSimpleText(const std::string& text, Vector2 pos, float fontSize, Color col);

   
    void DrawTextCentered(const std::string& text, Vector2 centerPos, float fontSize, Color col);

  
    void DrawTextFitted(const std::string& text, Vector2 centerPos, float maxWidth, float fontSize, Color col);

  
    float Measure(const std::string& text, float fontSize) const;

private:
    Font m_Font;
    float m_Spacing = 1.0f;

    void InitFont();
    std::string FitTextToWidth(const std::string& text, float maxWidth, float fontSize);
};