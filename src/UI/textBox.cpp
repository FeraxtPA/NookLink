
// Implementation of the TextBox widget class.
// Handles multi-line text editing with cursor movement and scroll management.


#include "textBox.h"
#include "../textRenderer.h"
#include "../colors.h"
#include "../utf8_utils.h"
#include <algorithm>


TextBox::TextBox(Anchor anchor, Vector2 offset, Vector2 size, std::string ph)
    : Widget(anchor, offset, size), placeholder(ph)
{
    OnWindowResize(GetScreenWidth(), GetScreenHeight());
}

void TextBox::MoveLeft(bool jumpWord) {
    if (cursorIndex == 0) return;

    if (!jumpWord) {
        cursorIndex = (int)Utf8::PrevCodepointStart(text, (size_t)cursorIndex);
    }
    else {
        size_t cursor = (size_t)cursorIndex;

        while (cursor > 0) {
            const size_t prevStart = Utf8::PrevCodepointStart(text, cursor);
            const int cp = Utf8::DecodeCodepointAt(text, prevStart);
            if (Utf8::IsWordCodepoint(cp)) break;
            cursor = prevStart;
        }

        while (cursor > 0) {
            const size_t prevStart = Utf8::PrevCodepointStart(text, cursor);
            const int cp = Utf8::DecodeCodepointAt(text, prevStart);
            if (!Utf8::IsWordCodepoint(cp)) break;
            cursor = prevStart;
        }

        cursorIndex = (int)cursor;
    }
}

void TextBox::MoveRight(bool jumpWord) {
    if (cursorIndex >= (int)text.length()) return;

    if (!jumpWord) {
        cursorIndex = (int)Utf8::NextCodepointStart(text, (size_t)cursorIndex);
    }
    else {
        size_t cursor = (size_t)cursorIndex;

        while (cursor < text.size()) {
            const int cp = Utf8::DecodeCodepointAt(text, cursor);
            if (!Utf8::IsWordCodepoint(cp)) break;
            cursor = Utf8::NextCodepointStart(text, cursor);
        }

        while (cursor < text.size()) {
            const int cp = Utf8::DecodeCodepointAt(text, cursor);
            if (Utf8::IsWordCodepoint(cp)) break;
            cursor = Utf8::NextCodepointStart(text, cursor);
        }

        cursorIndex = (int)cursor;
    }
}

//Key hold to move cursor
void TextBox::HandleKeyRepeat(int key, bool jumpWord, void (TextBox::* moveFunc)(bool)) {
    if (IsKeyPressed(key)) {
        (this->*moveFunc)(jumpWord);
        lastKeyPressed = key;
        keyRepeatTimer = 0.0f;
    }
    else if (IsKeyDown(key) && lastKeyPressed == key) {
        keyRepeatTimer += GetFrameTime();
        if (keyRepeatTimer > KEY_REPEAT_DELAY) {
            // Apply movement multiple times if frame time > repeat rate
            while (keyRepeatTimer > KEY_REPEAT_DELAY + KEY_REPEAT_RATE) {
                (this->*moveFunc)(jumpWord);
                keyRepeatTimer -= KEY_REPEAT_RATE;
            }
        }
    }
    else if (IsKeyReleased(key) && lastKeyPressed == key) {
        lastKeyPressed = -1;
        keyRepeatTimer = 0.0f;
    }
}

void TextBox::Update() {
    if (!isVisible) return;

    const Vector2 mouse = GetMousePosition();
    isHovered = CheckCollisionPointRec(mouse, m_Bounds);

    if (isEditable) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            const bool wasFocused = isFocused;
            isFocused = isHovered;

            if (isFocused && !wasFocused) cursorIndex = (int)text.size();
        }
    }

    cursorIndex = std::clamp(cursorIndex, 0, (int)text.size());

    // Wheel scrolling works regardless of editability.
    if (isHovered || isFocused) {
        const float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            scrollY -= wheel * 20.0f;
            // Once user scrolls manually, never auto-jump back to caret.
            m_UserScrolledManually = true;
        }
    }

    if (isEditable && isFocused) {

        Widget::DesiredCursor = MOUSE_CURSOR_IBEAM;

        const bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);

        HandleKeyRepeat(KEY_LEFT, ctrl, &TextBox::MoveLeft);
        HandleKeyRepeat(KEY_RIGHT, ctrl, &TextBox::MoveRight);

        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 32 && Utf8::CodepointCount(text) < (size_t)maxLength) {
                std::string utf8Char;
                Utf8::AppendCodepoint(utf8Char, key);
                text.insert((size_t)cursorIndex, utf8Char);
                cursorIndex += (int)utf8Char.size();
            }
            key = GetCharPressed();
        }

       
        if (IsKeyPressed(KEY_BACKSPACE)) {
            if (cursorIndex > 0 && !text.empty()) {
                int deleteCount = 0;
                int startIndex = cursorIndex;

                if (ctrl) {
                    size_t tempIndex = (size_t)cursorIndex;
                    while (tempIndex > 0) {
                        const size_t prev = Utf8::PrevCodepointStart(text, tempIndex);
                        const int cp = Utf8::DecodeCodepointAt(text, prev);
                        if (Utf8::IsWordCodepoint(cp)) break;
                        tempIndex = prev;
                    }
                    while (tempIndex > 0) {
                        const size_t prev = Utf8::PrevCodepointStart(text, tempIndex);
                        const int cp = Utf8::DecodeCodepointAt(text, prev);
                        if (!Utf8::IsWordCodepoint(cp)) break;
                        tempIndex = prev;
                    }

                    startIndex = (int)tempIndex;
                    deleteCount = cursorIndex - startIndex;
                }
                else {
                    const size_t prev = Utf8::PrevCodepointStart(text, (size_t)cursorIndex);
                    startIndex = (int)prev;
                    deleteCount = cursorIndex - startIndex;
                }

                text.erase((size_t)startIndex, (size_t)deleteCount);
                cursorIndex = startIndex;
            }
        }

        if (IsKeyPressed(KEY_DELETE)) {
            if (cursorIndex < (int)text.length()) {
                const size_t start = (size_t)cursorIndex;
                const size_t end = Utf8::NextCodepointStart(text, start);
                text.erase(start, end - start);
            }
        }

        if (IsKeyPressed(KEY_ENTER)) {
            text.insert((size_t)cursorIndex, 1, '\n');
            cursorIndex++;
        }
    } else if (isHovered) {
        Widget::DesiredCursor = MOUSE_CURSOR_POINTING_HAND;
    }
}

void TextBox::Draw(TextRenderer* renderer) {
    if (!isVisible) return;

    //Background & Border
    DrawRectangleRounded(m_Bounds, 0.14f, 10, NookCol::UI_PANEL_ALT);
    Color borderColor = isFocused ? NookCol::UI_ACCENT : (isHovered ? NookCol::UI_BORDER : NookCol::UI_BORDER_SOFT);
    DrawRectangleRoundedLinesEx(m_Bounds, 0.14f, 10, 2.0f, borderColor);

    if (!renderer) return;

    //Text to Draw
    std::string textToDraw = text;
    bool showPlaceholder = text.empty() && !isFocused;
    if (showPlaceholder) textToDraw = placeholder;

    
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
        float absoluteY = m_Bounds.y + padding + currentY - scrollY;
        float absoluteX = m_Bounds.x + padding;

        // Draw if visible
        if (absoluteY + lineHeight > m_Bounds.y && absoluteY < m_Bounds.y + m_Bounds.height) {
            std::string lineStr = textToDraw.substr(lineStartIndex, endIndex - lineStartIndex);
            renderer->DrawSimpleText(lineStr, { absoluteX, absoluteY }, fontSize, showPlaceholder ? NookCol::UI_TEXT_MUTED : NookCol::UI_TEXT);
        }

        // Track cursor against wrapped line segment so caret aligns with wrapped rendering.
        if (isFocused && !showPlaceholder && !cursorFound && cursorIndex >= lineStartIndex && cursorIndex <= endIndex) {
            
            std::string sub = textToDraw.substr(lineStartIndex, cursorIndex - lineStartIndex);
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

    
    if (isFocused && cursorFound && !m_UserScrolledManually) {
       
        float relY = cursorPos.y - m_Bounds.y;
        if (relY < 0) scrollY += relY;
        if (relY + lineHeight > m_Bounds.height) scrollY += (relY + lineHeight - m_Bounds.height);

        // Blinking
        if (((int)(GetTime() * 2) % 2) == 0) {
            renderer->DrawSimpleText("|", { cursorPos.x - 1, cursorPos.y }, fontSize, NookCol::UI_TEXT);
        }
    }

    EndScissorMode();

    //Scrollbar Logic
    float totalHeight = currentY + lineHeight;
    maxScrollY = std::max(0.0f, totalHeight - (m_Bounds.height - padding * 2));

    // Clamp scroll
    if (scrollY < 0) scrollY = 0;
    if (scrollY > maxScrollY) scrollY = maxScrollY;

    if (maxScrollY > 0) {
        float scrollPerc = scrollY / maxScrollY;
        float barHeight = std::max(20.0f, (m_Bounds.height / totalHeight) * m_Bounds.height);
        float barY = m_Bounds.y + (scrollPerc * (m_Bounds.height - barHeight));
        DrawRectangle((int)(m_Bounds.x + m_Bounds.width - 6), (int)barY, 4, (int)barHeight, Fade(NookCol::UI_ACCENT_SOFT, 0.45f));
    }
}