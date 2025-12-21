#include "checkbox.h"
#include "../textRenderer.h"

Checkbox::Checkbox(Rectangle r, std::string l, bool initial)
    : Widget(r), label(l), checked(initial)
{}

void Checkbox::Update() {
    if (!isVisible) return;

    // Check collision with the box AND the label (approximate label width)
    // For simplicity, we just check the box, but you can expand 'bounds' if needed.
    Rectangle clickArea = bounds;
    clickArea.width += 200; // Allow clicking the text too

    if (CheckCollisionPointRec(GetMousePosition(), clickArea)) {
        isHovered = true;
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            checked = !checked;
        }
    }
    else {
        isHovered = false;
    }
}

void Checkbox::Draw(TextRenderer* renderer) {
    if (!isVisible) return;

    // 1. Draw Box Background
    DrawRectangleRec(bounds, RAYWHITE);

    // 2. Draw Border (Red if focused/hovered, Gray otherwise)
    Color borderColor = isHovered ? DARKGRAY : LIGHTGRAY;
    DrawRectangleLinesEx(bounds, 2, borderColor);

    // 3. Draw Checkmark (Filled square)
    if (checked) {
        DrawRectangle((int)bounds.x + 4, (int)bounds.y + 4, (int)bounds.width - 8, (int)bounds.height - 8, DARKGRAY);
    }

    // 4. Draw Label
    if (renderer) {
        renderer->DrawSimpleText(label, { bounds.x + bounds.width + 10, bounds.y }, 20, BLACK);
    }
}