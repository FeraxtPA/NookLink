#pragma once
#include "widget.h"
#include <string>
#include <vector>

class Label : public Widget {
public:
    std::string text;
    int fontSize;
    Color color;
    bool wordWrap;

    // Zmìna: Místo maxWidth pøedáváme rovnou celou size (Vector2)
    Label(Anchor anchor, Vector2 offset, Vector2 size, std::string t, int fSize = 20, Color c = DARKGRAY, bool wrap = false);

    void Update() override;
    void Draw(TextRenderer* renderer) override;
    void SetText(const std::string& t) { text = t; }

private:
    std::string m_LastText;
    std::vector<std::string> m_WrappedLines;

    // Promìnné pro scrollování
    float m_ScrollY = 0.0f;
    float m_ContentHeight = 0.0f;
};