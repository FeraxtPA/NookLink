#include "textInput.h"
#include "../textRenderer.h" 

TextInput::TextInput(Anchor anchor, Vector2 offset, Vector2 size, std::string ph)
    : Widget(anchor, offset, size), placeholder(ph)
{
    OnWindowResize(GetScreenWidth(), GetScreenHeight());
}
void TextInput::Update() {
    if (!isVisible) return;

    Vector2 mouse = GetMousePosition();
    bool isHovered = CheckCollisionPointRec(mouse, m_Bounds);

   
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        isFocused = isHovered;
    }

    
    if (isFocused) {
        Widget::DesiredCursor = MOUSE_CURSOR_IBEAM; 

        int key = GetCharPressed();
        while (key > 0) {
            // Only allow printable characters and check length
            if ((key >= 32) && (key <= 125) && (text.length() < maxLength)) {
                text.push_back((char)key);
            }
            key = GetCharPressed();
        }

        // Handle Backspace
        if (IsKeyPressed(KEY_BACKSPACE) && !text.empty()) {
            text.pop_back();
        }
    }
    else if (isHovered) {
        Widget::DesiredCursor = MOUSE_CURSOR_POINTING_HAND; 
    }
    else
    {
        Widget::DesiredCursor = MOUSE_CURSOR_DEFAULT;
    }
	
    
}

void TextInput::Draw(TextRenderer* renderer) {
    if (!isVisible) return;

    // Draw Background + Border
    DrawRectangleRec(m_Bounds, RAYWHITE);

    Color borderColor = isFocused ? RED : (CheckCollisionPointRec(GetMousePosition(), m_Bounds) ? DARKGRAY : LIGHTGRAY);
    DrawRectangleLinesEx(m_Bounds, 2, borderColor);

    if (!renderer) return;

    const float fontSize = 20.0f;
    const float paddingX = 5.0f;
    const float paddingY = 8.0f;

    Vector2 textPos = { m_Bounds.x + paddingX, m_Bounds.y + paddingY };

    // Draw Text or Placeholder
    if (text.empty() && !isFocused) {
        renderer->DrawSimpleText(placeholder, textPos, fontSize, GRAY);
    }
    else {
        renderer->DrawSimpleText(text, textPos, fontSize, BLACK);
    }

    // Draw Blinking Cursor
    if (isFocused && ((int)(GetTime() * 2) % 2) == 0) {
        float textWidth = renderer->Measure(text, fontSize);

        Vector2 cursorPos = { textPos.x + textWidth + 2, textPos.y };
        renderer->DrawSimpleText("|", cursorPos, fontSize, BLACK);
    }
}