
// Single-line text input widget.
// Provides text editing with selection, focus management, and placeholder text.


#pragma once
#include "widget.h"
#include "../constants.h"
#include <string>

class TextInput : public Widget {
public:
    std::string text;
    std::string placeholder;
    bool isFocused = false;
    bool hasValidationError = false;
    int maxLength = 50; 

    TextInput(Anchor anchor, Vector2 offset, Vector2 size, std::string ph = "");

    void Update() override;
    void Draw(TextRenderer* renderer) override;

    
    std::string GetText() const { return text; }
    void Clear() { text.clear(); isFocused = false; }
    void SetValidationError(bool value) { hasValidationError = value; }

private:
    float m_BackspaceRepeatTimer = 0.0f;
    bool m_BackspaceRepeatArmed = false;
    static constexpr float kBackspaceRepeatDelay = NookConst::Input::kTextInputBackspaceRepeatDelay;
    static constexpr float kBackspaceRepeatRate = NookConst::Input::kTextInputBackspaceRepeatRate;
};