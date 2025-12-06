#include "textInput.h"
#include "../textRenderer.h" 

TextInput::TextInput(Rectangle r, std::string ph)
    : Widget(r), placeholder(ph)
{}

void TextInput::Update() {
    if (!isVisible) return;

    Vector2 mouse = GetMousePosition();
    bool isHovered = CheckCollisionPointRec(mouse, bounds);

    // 1. Handle Focus
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        isFocused = isHovered;
    }

    // 2. Handle Text Entry
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
	
    
}

void TextInput::Draw(TextRenderer* renderer) {
    if (!isVisible) return;

    // Draw Background & Border
    DrawRectangleRec(bounds, RAYWHITE);

    Color borderColor = isFocused ? RED : (CheckCollisionPointRec(GetMousePosition(), bounds) ? DARKGRAY : LIGHTGRAY);
    DrawRectangleLinesEx(bounds, 2, borderColor);

    if (!renderer) return;

    const float fontSize = 20.0f;
    const float paddingX = 5.0f;
    const float paddingY = 8.0f;

    Vector2 textPos = { bounds.x + paddingX, bounds.y + paddingY };

    // Draw Text or Placeholder
    if (text.empty() && !isFocused) {
        renderer->DrawSimpleText(placeholder, textPos, fontSize, GRAY);
    }
    else {
        renderer->DrawSimpleText(text, textPos, fontSize, BLACK);
    }

    // Draw Blinking Cursor
    // We use renderer->Measure to find exactly where the text ends
    if (isFocused && ((int)(GetTime() * 2) % 2) == 0) {
        float textWidth = renderer->Measure(text, fontSize);

        Vector2 cursorPos = { textPos.x + textWidth + 2, textPos.y };
        renderer->DrawSimpleText("|", cursorPos, fontSize, BLACK);
    }
}