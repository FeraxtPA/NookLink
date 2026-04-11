
// Implementation of the TextBox widget class.
// Handles multi-line text editing with cursor movement and scroll management.


#include "textBox.h"
#include "../textRenderer.h"
#include "../colors.h"
#include "../utf8_utils.h"
#include <algorithm>

namespace {
std::string BuildTextBoxClipboardInsert(size_t maxCodepointsToAdd)
{
    if (maxCodepointsToAdd == 0) {
        return "";
    }

	// Get raw clipboard text and filter out control characters except newlines.
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

        if (cp == '\r') {
            i = next;
            continue;
        }

        if (cp == '\n' || cp >= 32) {
            Utf8::AppendCodepoint(result, cp);
            ++added;
        }

        i = next;
    }

    return result;
}
}


TextBox::TextBox(Anchor anchor, Vector2 offset, Vector2 size, std::string ph)
    : Widget(anchor, offset, size), m_PlaceHolder(ph)
{
    OnWindowResize(GetScreenWidth(), GetScreenHeight());
}

// Moves cursor left, optionally jumping over entire words when ctrl is held.
void TextBox::MoveLeft(bool jumpWord) {
    if (m_CursorIndex == 0) return;

    if (!jumpWord) {
        m_CursorIndex = (int)Utf8::PrevCodepointStart(m_Text, (size_t)m_CursorIndex);
    }
    else {
        size_t cursor = (size_t)m_CursorIndex;

        while (cursor > 0) {
            const size_t prevStart = Utf8::PrevCodepointStart(m_Text, cursor);
            const int cp = Utf8::DecodeCodepointAt(m_Text, prevStart);
            if (Utf8::IsWordCodepoint(cp)) break;
            cursor = prevStart;
        }

        while (cursor > 0) {
            const size_t prevStart = Utf8::PrevCodepointStart(m_Text, cursor);
            const int cp = Utf8::DecodeCodepointAt(m_Text, prevStart);
            if (!Utf8::IsWordCodepoint(cp)) break;
            cursor = prevStart;
        }

        m_CursorIndex = (int)cursor;
    }
}

// Moves cursor right, optionally jumping over entire words when ctrl is held.
void TextBox::MoveRight(bool jumpWord) {
    if (m_CursorIndex >= (int)m_Text.length()) return;

    if (!jumpWord) {
        m_CursorIndex = (int)Utf8::NextCodepointStart(m_Text, (size_t)m_CursorIndex);
    }
    else {
        size_t cursor = (size_t)m_CursorIndex;

        while (cursor < m_Text.size()) {
            const int cp = Utf8::DecodeCodepointAt(m_Text, cursor);
            if (!Utf8::IsWordCodepoint(cp)) break;
            cursor = Utf8::NextCodepointStart(m_Text, cursor);
        }

        while (cursor < m_Text.size()) {
            const int cp = Utf8::DecodeCodepointAt(m_Text, cursor);
            if (Utf8::IsWordCodepoint(cp)) break;
            cursor = Utf8::NextCodepointStart(m_Text, cursor);
        }

        m_CursorIndex = (int)cursor;
    }
}

//Key hold to move cursor
void TextBox::HandleKeyRepeat(int key, bool jumpWord, void (TextBox::* moveFunc)(bool)) {
    if (IsKeyPressed(key)) {
        (this->*moveFunc)(jumpWord);
        m_LastKeyPressed = key;
        m_KeyRepeatTimer = 0.0f;
    }
    else if (IsKeyDown(key) && m_LastKeyPressed == key) {
        m_KeyRepeatTimer += GetFrameTime();
        if (m_KeyRepeatTimer > KEY_REPEAT_DELAY) {
            // Apply movement multiple times if frame time > repeat rate
            while (m_KeyRepeatTimer > KEY_REPEAT_DELAY + KEY_REPEAT_RATE) {
                (this->*moveFunc)(jumpWord);
                m_KeyRepeatTimer -= KEY_REPEAT_RATE;
            }
        }
    }
    else if (IsKeyReleased(key) && m_LastKeyPressed == key) {
        m_LastKeyPressed = -1;
        m_KeyRepeatTimer = 0.0f;
    }
}

void TextBox::Update() {
    if (!m_IsVisible) return;

    const Vector2 mouse = GetMousePosition();
    m_IsHovered = CheckCollisionPointRec(mouse, m_Bounds);
    if (m_IsEditable && (m_IsHovered || m_IsFocused)) {
        RequestCursor(MOUSE_CURSOR_IBEAM);
    }

    if (m_IsEditable) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            const bool wasFocused = m_IsFocused;
            m_IsFocused = m_IsHovered;

            if (m_IsFocused && !wasFocused) m_CursorIndex = (int)m_Text.size();
        }
    }

    m_CursorIndex = std::clamp(m_CursorIndex, 0, (int)m_Text.size());

    // Wheel scrolling works regardless of editability.
    if (m_IsHovered || m_IsFocused) {
        const float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            m_ScrollY -= wheel * 20.0f;
            // Once user scrolls manually, never auto-jump back to caret.
            m_UserScrolledManually = true;
        }
    }

    if (m_IsEditable && m_IsFocused) {

        const bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);

        if (ctrl && IsKeyPressed(KEY_C) && !m_Text.empty()) {
            SetClipboardText(m_Text.c_str());
        }

        if (ctrl && IsKeyPressed(KEY_V)) {
            const size_t currentCount = Utf8::CodepointCount(m_Text);
            const size_t remaining = currentCount < (size_t)m_MaxLength ? (size_t)m_MaxLength - currentCount : 0;
            const std::string insert = BuildTextBoxClipboardInsert(remaining);
            if (!insert.empty()) {
                m_Text.insert((size_t)m_CursorIndex, insert);
                m_CursorIndex += (int)insert.size();
            }
        }

        HandleKeyRepeat(KEY_LEFT, ctrl, &TextBox::MoveLeft);
        HandleKeyRepeat(KEY_RIGHT, ctrl, &TextBox::MoveRight);

        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 32 && Utf8::CodepointCount(m_Text) < (size_t)m_MaxLength) {
                std::string utf8Char;
                Utf8::AppendCodepoint(utf8Char, key);
                m_Text.insert((size_t)m_CursorIndex, utf8Char);
                m_CursorIndex += (int)utf8Char.size();
            }
            key = GetCharPressed();
        }

       
        if (IsKeyPressed(KEY_BACKSPACE)) {
            if (m_CursorIndex > 0 && !m_Text.empty()) {
                int deleteCount = 0;
                int startIndex = m_CursorIndex;

                if (ctrl) {
                    size_t tempIndex = (size_t)m_CursorIndex;
                    while (tempIndex > 0) {
                        const size_t prev = Utf8::PrevCodepointStart(m_Text, tempIndex);
                        const int cp = Utf8::DecodeCodepointAt(m_Text, prev);
                        if (Utf8::IsWordCodepoint(cp)) break;
                        tempIndex = prev;
                    }
                    while (tempIndex > 0) {
                        const size_t prev = Utf8::PrevCodepointStart(m_Text, tempIndex);
                        const int cp = Utf8::DecodeCodepointAt(m_Text, prev);
                        if (!Utf8::IsWordCodepoint(cp)) break;
                        tempIndex = prev;
                    }

                    startIndex = (int)tempIndex;
                    deleteCount = m_CursorIndex - startIndex;
                }
                else {
                    const size_t prev = Utf8::PrevCodepointStart(m_Text, (size_t)m_CursorIndex);
                    startIndex = (int)prev;
                    deleteCount = m_CursorIndex - startIndex;
                }

                m_Text.erase((size_t)startIndex, (size_t)deleteCount);
                m_CursorIndex = startIndex;
            }
        }

        if (IsKeyPressed(KEY_DELETE)) {
            if (m_CursorIndex < (int)m_Text.length()) {
                const size_t start = (size_t)m_CursorIndex;
                const size_t end = Utf8::NextCodepointStart(m_Text, start);
                m_Text.erase(start, end - start);
            }
        }

        if (IsKeyPressed(KEY_ENTER)) {
            m_Text.insert((size_t)m_CursorIndex, 1, '\n');
            m_CursorIndex++;
        }
    } else if (m_IsHovered) {
		RequestCursor(MOUSE_CURSOR_IBEAM);
    }
}

void TextBox::Draw(TextRenderer* renderer) {
    if (!m_IsVisible) return;

    //Background & Border
    DrawRectangleRounded(m_Bounds, 0.14f, 10, NookCol::UI_PANEL_ALT);
    Color borderColor = m_IsFocused ? NookCol::UI_ACCENT : (m_IsHovered ? NookCol::UI_BORDER : NookCol::UI_BORDER_SOFT);
    DrawRectangleRoundedLinesEx(m_Bounds, 0.14f, 10, 2.0f, borderColor);

    if (!renderer) return;

    //Text to Draw
    std::string textToDraw = m_Text;
    bool showPlaceholder = m_Text.empty() && !m_IsFocused;
    if (showPlaceholder) textToDraw = m_PlaceHolder;

    
    const float fontSize = 20.0f;
    const float padding = 5.0f;
    const float lineHeight = 24.0f;
    const float contentWidth = m_Bounds.width - (padding * 2);

    float currentX = 0.0f;
    float currentY = 0.0f;

    // Track cursor visualization
    Vector2 cursorPos = { -1, -1 };
    bool cursorFound = false;

    // Track word wrapping state
    int lineStartIndex = 0;
    int lastSpaceIndex = -1; // Index within the whole string

    // Helper to "commit" a line to drawing
    auto finishLine = [&](int endIndex, bool newline) {
        float absoluteY = m_Bounds.y + padding + currentY - m_ScrollY;
        float absoluteX = m_Bounds.x + padding;

        // Draw if visible
        if (absoluteY + lineHeight > m_Bounds.y && absoluteY < m_Bounds.y + m_Bounds.height) {
            std::string lineStr = textToDraw.substr(lineStartIndex, endIndex - lineStartIndex);
            renderer->DrawSimpleText(lineStr, { absoluteX, absoluteY }, fontSize, showPlaceholder ? NookCol::UI_TEXT_MUTED : NookCol::UI_TEXT);
        }

        // Track cursor against wrapped line segment so caret aligns with wrapped rendering.
        if (m_IsFocused && !showPlaceholder && !cursorFound && m_CursorIndex >= lineStartIndex && m_CursorIndex <= endIndex) {
            
            std::string sub = textToDraw.substr(lineStartIndex, m_CursorIndex - lineStartIndex);
            float subWidth = renderer->Measure(sub, fontSize);
            cursorPos = { absoluteX + subWidth, absoluteY };
            cursorFound = true;
        }

        currentY += lineHeight;
        currentX = 0.0f;
        };

    BeginScissorMode((int)m_Bounds.x, (int)m_Bounds.y, (int)m_Bounds.width, (int)m_Bounds.height);

    size_t i = 0;
    while (i < textToDraw.size()) {
        int cpSize = 0;
        const int cp = Utf8::DecodeCodepointAt(textToDraw, i, &cpSize);
        const size_t next = std::min(textToDraw.size(), i + (size_t)std::max(cpSize, 1));

        // Handle newline
        if (cp == '\n') {
            finishLine((int)i, true);
            lineStartIndex = (int)next;
            lastSpaceIndex = -1;
            i = next;
            continue;
        }

        const std::string glyph = textToDraw.substr(i, next - i);
        const float glyphWidth = renderer->Measure(glyph, fontSize);

        // Soft-wrap at last whitespace when possible; hard-wrap long unbroken tokens.
        if (currentX + glyphWidth > contentWidth) {
            if (lastSpaceIndex != -1 && lastSpaceIndex > lineStartIndex) {
                finishLine(lastSpaceIndex, false);
                i = (size_t)lastSpaceIndex;
                lineStartIndex = (int)i;
                lastSpaceIndex = -1;
            }
            else {
                finishLine((int)i, false);
                lineStartIndex = (int)i;
            }
            continue;
        }

        currentX += glyphWidth;
        if (Utf8::IsWhitespaceCodepoint(cp)) {
            lastSpaceIndex = (int)next;
        }
        i = next;
    }

  
    finishLine((int)textToDraw.length(), false);

    
    if (m_IsFocused && cursorFound && !m_UserScrolledManually) {
       
        float relY = cursorPos.y - m_Bounds.y;
        if (relY < 0) m_ScrollY += relY;
        if (relY + lineHeight > m_Bounds.height) m_ScrollY += (relY + lineHeight - m_Bounds.height);

        // Blinking
        if (((int)(GetTime() * 2) % 2) == 0) {
            renderer->DrawSimpleText("|", { cursorPos.x - 1, cursorPos.y }, fontSize, NookCol::UI_TEXT);
        }
    }

    EndScissorMode();

    //Scrollbar Logic
    float totalHeight = currentY + lineHeight;
    m_MaxScrollY = std::max(0.0f, totalHeight - (m_Bounds.height - padding * 2));

    // Clamp scroll
    if (m_ScrollY < 0) m_ScrollY = 0;
    if (m_ScrollY > m_MaxScrollY) m_ScrollY = m_MaxScrollY;

    if (m_MaxScrollY > 0) {
        float scrollPerc = m_ScrollY / m_MaxScrollY;
        float barHeight = std::max(20.0f, (m_Bounds.height / totalHeight) * m_Bounds.height);
        float barY = m_Bounds.y + (scrollPerc * (m_Bounds.height - barHeight));
        DrawRectangle((int)(m_Bounds.x + m_Bounds.width - 6), (int)barY, 4, (int)barHeight, Fade(NookCol::UI_ACCENT_SOFT, 0.45f));
    }
}