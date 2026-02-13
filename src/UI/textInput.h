#pragma once
#include "widget.h"
#include <string>

class TextInput : public Widget {
public:
    std::string text;
    std::string placeholder;
    bool isFocused = false;
    int maxLength = 50; 

    TextInput(Anchor anchor, Vector2 offset, Vector2 size, std::string ph = "");

    void Update() override;
    void Draw(TextRenderer* renderer) override;

    
    std::string GetText() const { return text; }
    void Clear() { text.clear(); isFocused = false; }
};