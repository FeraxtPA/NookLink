#include "button.h"
#include "../textRenderer.h" 

Button::Button(Anchor anchor, Vector2 offset, Vector2 size, std::string t, std::function<void()> callback)
    : Widget(anchor, offset, size), text(t), onClick(callback)
{
    // Vypoèítat poèáteèní Rectangle (m_Bounds) ihned po vytvoøení
    OnWindowResize(GetScreenWidth(), GetScreenHeight());
}

void Button::Update() {
    if (!isVisible) return;
    Vector2 mouse = GetMousePosition();

    isHovered = CheckCollisionPointRec(mouse, m_Bounds);

    if (isHovered) {
       
        
        Widget::DesiredCursor = MOUSE_CURSOR_POINTING_HAND;

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && onClick) {
            onClick();
        }
    }
    
}

void Button::Draw(TextRenderer* renderer) {
    if (!isVisible) return;

    // Draw Box
    DrawRectangleRec(m_Bounds, isHovered ? DARKGRAY : LIGHTGRAY);
    DrawRectangleLinesEx(m_Bounds, 2, BLACK);

    
    if (renderer) {
        Vector2 center = { m_Bounds.x + m_Bounds.width / 2.0f, m_Bounds.y + m_Bounds.height / 2.0f };
        renderer->DrawTextCentered(text, center, 20.0f, BLACK);
    }
}