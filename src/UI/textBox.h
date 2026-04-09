
// Multi-line text box widget with scrolling.
// Provides text editing, line wrapping, and scroll management for large text.


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

    TextBox(Anchor anchor, Vector2 offset, Vector2 size, std::string ph = "");

    void Update() override;
    void Draw(TextRenderer* renderer) override;

    std::string GetText() const { return text; }
    void SetText(const std::string& t) { text = t; cursorIndex = (int)t.size(); scrollY = 0.0f; m_UserScrolledManually = false; }
    void Clear() { text.clear(); cursorIndex = 0; scrollY = 0.0f; isFocused = false; m_UserScrolledManually = false; }
    void SetEditable(bool editable) { isEditable = editable; }

private:

    bool isEditable = true;
    bool m_UserScrolledManually = false;
    void MoveLeft(bool jumpWord);
    void MoveRight(bool jumpWord);
    void HandleKeyRepeat(int key, bool jumpWord, void (TextBox::* moveFunc)(bool));
};