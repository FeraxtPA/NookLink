
// Multi-line text box widget with scrolling.
// Provides text editing, line wrapping, and scroll management for large text.


#pragma once
#include "widget.h"
#include "../constants.h"
#include <string>
#include <vector>

class TextBox : public Widget {
public:
    

    TextBox(Anchor anchor, Vector2 offset, Vector2 size, std::string ph = "");

    void Update() override;
    void Draw(TextRenderer* renderer) override;

    std::string GetText() const { return m_Text; }
    void SetText(const std::string& t) { m_Text = t; m_CursorIndex = (int)t.size(); m_ScrollY = 0.0f; m_UserScrolledManually = false; }
    void Clear() { m_Text.clear(); m_CursorIndex = 0; m_ScrollY = 0.0f; m_IsFocused = false; m_UserScrolledManually = false; }
    void SetEditable(bool editable) { m_IsEditable = editable; }

	bool IsFocused() const { return m_IsFocused; }
	void SetFocused(bool focused) { m_IsFocused = focused; if (focused) m_CursorIndex = (int)m_Text.size(); }

private:
    int m_MaxLength = 2000;

    bool m_IsFocused = false;

    static constexpr float KEY_REPEAT_RATE = NookConst::Input::kTextBoxKeyRepeatRate;
    static constexpr float KEY_REPEAT_DELAY = NookConst::Input::kTextBoxKeyRepeatDelay;
    float m_KeyRepeatTimer = 0.0f;

    int m_LastKeyPressed = -1;

    int m_CursorIndex = 0;

    std::string m_PlaceHolder;

    float m_ScrollY = 0.0f;
    float m_MaxScrollY = 0.0f;

    std::string m_Text;
    bool m_IsEditable = true;
    bool m_UserScrolledManually = false;
    void MoveLeft(bool jumpWord);
    void MoveRight(bool jumpWord);
    void HandleKeyRepeat(int key, bool jumpWord, void (TextBox::* moveFunc)(bool));
};