#include "checkbox.h"
#include "../textRenderer.h"

Checkbox::Checkbox(Rectangle r, std::string l, bool initial, std::function<void(bool)> onChangeCallback)
    : Widget(r), label(l), checked(initial), onChange(onChangeCallback)
{}

void Checkbox::Update() {
    if (!isVisible) return;

    
    Rectangle clickArea = bounds;
    clickArea.width += 200; // Allow clicking the text too

    if (CheckCollisionPointRec(GetMousePosition(), clickArea)) {
        isHovered = true;
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            checked = !checked;
            if (onChange) {
                onChange(checked);
            }
        }
    }
    else {
        isHovered = false;
    }
}

void Checkbox::Draw(TextRenderer* renderer) {
    if (!isVisible) return;

    
    DrawRectangleRec(bounds, RAYWHITE);

  
    Color borderColor = isHovered ? DARKGRAY : LIGHTGRAY;
    DrawRectangleLinesEx(bounds, 2, borderColor);

  
    if (checked) {
        DrawRectangle((int)bounds.x + 4, (int)bounds.y + 4, (int)bounds.width - 8, (int)bounds.height - 8, DARKGRAY);
    }

  
    if (renderer) {
        renderer->DrawSimpleText(label, { bounds.x + bounds.width + 10, bounds.y }, 20, BLACK);
    }
}