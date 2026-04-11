
// Implementation of the TextInput widget class.
// Handles text input, cursor management, and focus states.


#include "textInput.h"
#include "../textRenderer.h" 
#include "../colors.h"
#include "../utf8_utils.h"

namespace {
constexpr Color kValidationErrorColor = { 226, 92, 92, 255 };

std::string BuildSingleLineClipboardInsert(size_t maxCodepointsToAdd)
{
    if (maxCodepointsToAdd == 0) {
        return "";
    }

    const char* clipboard = GetClipboardText();
    if (!clipboard || clipboard[0] == '\0') {
        return "";
    }

    const std::string source(clipboard);
    std::string result;
    size_t added = 0;

    size_t i = 0;
    while (i < source.size() && added < maxCodepointsToAdd) {
        int cpSize = 0;
        const int cp = Utf8::DecodeCodepointAt(source, i, &cpSize);
        const size_t next = std::min(source.size(), i + (size_t)std::max(cpSize, 1));

        if (cp >= 32 && cp != '\n' && cp != '\r') {
            Utf8::AppendCodepoint(result, cp);
            ++added;
        }

        i = next;
    }

    return result;
}

void DeletePrevCodepoint(std::string& text)
{
    if (text.empty()) return;
    size_t cursor = text.size();
    Utf8::ErasePrevCodepoint(text, cursor);
}

void DeletePrevWord(std::string& text)
{
    if (text.empty()) return;

    size_t cursor = text.size();

    while (cursor > 0) {
        const size_t prev = Utf8::PrevCodepointStart(text, cursor);
        const int cp = Utf8::DecodeCodepointAt(text, prev);
        if (Utf8::IsWordCodepoint(cp)) break;
        cursor = prev;
    }

    while (cursor > 0) {
        const size_t prev = Utf8::PrevCodepointStart(text, cursor);
        const int cp = Utf8::DecodeCodepointAt(text, prev);
        if (!Utf8::IsWordCodepoint(cp)) break;
        cursor = prev;
    }

    text.erase(cursor);
}
}

TextInput::TextInput(Anchor anchor, Vector2 offset, Vector2 size, std::string ph)
    : Widget(anchor, offset, size), placeholder(ph)
{
    OnWindowResize(GetScreenWidth(), GetScreenHeight());
}
void TextInput::Update() {
    if (!m_IsVisible) return;

    Vector2 mouse = GetMousePosition();
    bool isHovered = CheckCollisionPointRec(mouse, m_Bounds);
    m_IsHovered = isHovered;

    if (m_IsHovered || isFocused) {
        RequestCursor(MOUSE_CURSOR_IBEAM);
    }

   
    // Focus is click-driven: click inside to focus, outside to blur.
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        isFocused = isHovered;
        if (!isFocused) {
            m_BackspaceRepeatTimer = 0.0f;
            m_BackspaceRepeatArmed = false;
        }
    }

    
    if (isFocused) {

        const bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);

        if (ctrl && IsKeyPressed(KEY_C) && !text.empty()) {
            SetClipboardText(text.c_str());
        }

        if (ctrl && IsKeyPressed(KEY_V)) {
            const size_t currentCount = Utf8::CodepointCount(text);
            const size_t remaining = currentCount < (size_t)maxLength ? (size_t)maxLength - currentCount : 0;
            const std::string insert = BuildSingleLineClipboardInsert(remaining);
            if (!insert.empty()) {
                text += insert;
            }
        }

        int key = GetCharPressed();
        while (key > 0) {
            // Accept printable Unicode codepoints and enforce max length by codepoints, not bytes.
            if (key >= 32 && Utf8::CodepointCount(text) < (size_t)maxLength) {
                Utf8::AppendCodepoint(text, key);
            }
            key = GetCharPressed();
        }

        auto applyBackspace = [&]() {
            if (text.empty()) return;
            if (ctrl) DeletePrevWord(text);
            else DeletePrevCodepoint(text);
        };

        if (IsKeyPressed(KEY_BACKSPACE)) {
            applyBackspace();
            m_BackspaceRepeatTimer = 0.0f;
            m_BackspaceRepeatArmed = true;
        }
        else if (IsKeyDown(KEY_BACKSPACE) && m_BackspaceRepeatArmed) {
            m_BackspaceRepeatTimer += GetFrameTime();
            if (m_BackspaceRepeatTimer > kBackspaceRepeatDelay) {
                while (m_BackspaceRepeatTimer > kBackspaceRepeatDelay + kBackspaceRepeatRate) {
                    applyBackspace();
                    m_BackspaceRepeatTimer -= kBackspaceRepeatRate;
                    if (text.empty()) {
                        m_BackspaceRepeatArmed = false;
                        m_BackspaceRepeatTimer = 0.0f;
                        break;
                    }
                }
            }
        }
        else if (IsKeyReleased(KEY_BACKSPACE)) {
            m_BackspaceRepeatTimer = 0.0f;
            m_BackspaceRepeatArmed = false;
        }
    }
    else if (isHovered) {
		RequestCursor(MOUSE_CURSOR_POINTING_HAND);  
    }
    else
    {
        RequestCursor(MOUSE_CURSOR_DEFAULT);
    }
	
    
}

void TextInput::Draw(TextRenderer* renderer) {
    if (!m_IsVisible) return;

    // Draw Background + Border
    DrawRectangleRounded(m_Bounds, 0.20f, 10, NookCol::UI_PANEL_ALT);

    Color borderColor = hasValidationError
        ? kValidationErrorColor
        : (isFocused ? NookCol::UI_ACCENT : (CheckCollisionPointRec(GetMousePosition(), m_Bounds) ? NookCol::UI_BORDER : NookCol::UI_BORDER_SOFT));
    DrawRectangleRoundedLinesEx(m_Bounds, 0.20f, 10, 2.0f, borderColor);

    if (!renderer) return;

    const float fontSize = 20.0f;
    const float paddingX = 5.0f;
    const float paddingY = 8.0f;

    Vector2 textPos = { m_Bounds.x + paddingX, m_Bounds.y + paddingY };

    // Draw Text or Placeholder
    if (text.empty() && !isFocused) {
        renderer->DrawSimpleText(placeholder, textPos, fontSize, NookCol::UI_TEXT_MUTED);
    }
    else {
        renderer->DrawSimpleText(text, textPos, fontSize, NookCol::UI_TEXT);
    }

    // Draw caret at measured text width to keep visual position consistent with font metrics.
    if (isFocused && ((int)(GetTime() * 2) % 2) == 0) {
        float textWidth = renderer->Measure(text, fontSize);

        Vector2 cursorPos = { textPos.x + textWidth + 2, textPos.y };
        renderer->DrawSimpleText("|", cursorPos, fontSize, NookCol::UI_TEXT);
    }
}