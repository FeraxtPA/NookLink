#include "button.h"
#include "../textRenderer.h" // Now needs full definition

Button::Button(Rectangle r, std::string t, std::function<void()> callback)
    : Widget(r), text(t), onClick(callback)
{
}

void Button::Update() {
    if (!isVisible) return;
    Vector2 mouse = GetMousePosition();
    isHovered = CheckCollisionPointRec(mouse, bounds);

    if (isHovered)
    {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    }
    else
    {
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    }
    if (isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && onClick) {
        onClick();
    }
}

void Button::Draw(TextRenderer* renderer) {
    if (!isVisible) return;

    // Draw Box
    DrawRectangleRec(bounds, isHovered ? DARKGRAY : LIGHTGRAY);
    DrawRectangleLinesEx(bounds, 2, BLACK);

    // Use the passed renderer
    if (renderer) {
        Vector2 center = { bounds.x + bounds.width / 2.0f, bounds.y + bounds.height / 2.0f };
        renderer->DrawTextCentered(text, center, 20.0f, BLACK);
    }
}