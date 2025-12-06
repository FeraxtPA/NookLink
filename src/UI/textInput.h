#pragma once
#include "Widget.h"
#include <string>

class TextInput : public Widget {
public:
    std::string text;
    std::string placeholder;
    bool isFocused = false;
    int maxLength = 30; // Maximum characters

    TextInput(Rectangle r, std::string ph = "");

    void Update() override;
    void Draw(TextRenderer* renderer) override;

    // Helper to get text out
    std::string GetText() const { return text; }
    void Clear() { text.clear(); isFocused = false; }
};