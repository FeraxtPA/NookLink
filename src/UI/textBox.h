#pragma once
#include "widget.h"
#include <string>
#include <vector>

class TextBox : public Widget {
public:
    std::string text;
    std::string placeholder;
    bool isFocused = false;
    int maxLength = 2000;

    
    int cursorIndex = 0;

   
    float scrollY = 0.0f;
    float maxScrollY = 0.0f;

    // Key Repeat State
    float keyRepeatTimer = 0.0f;
    int lastKeyPressed = -1;
    const float KEY_REPEAT_DELAY = 0.5f; 
    const float KEY_REPEAT_RATE = 0.03f; 

    TextBox(Rectangle r, std::string ph = "");

    void Update() override;
    void Draw(TextRenderer* renderer) override;

    std::string GetText() const { return text; }
    void SetText(const std::string& t) { text = t; cursorIndex = (int)t.length(); scrollY = 0.0f; }
    void Clear() { text.clear(); cursorIndex = 0; scrollY = 0.0f; isFocused = false; }
    void SetEditable(bool editable) { isEditable = editable; }

private:

    bool isEditable = true;
    void MoveLeft(bool jumpWord);
    void MoveRight(bool jumpWord);
    void HandleKeyRepeat(int key, bool jumpWord, void (TextBox::* moveFunc)(bool));
};