
// Implementation of the Button widget class.
// Provides clickable button with hover states and callbacks.


#include "button.h"
#include "../textRenderer.h" 
#include "../colors.h"

Button::Button(Anchor anchor, Vector2 offset, Vector2 size, std::string t, std::function<void()> callback)
    : Widget(anchor, offset, size), text(t), onClick(callback)
{
    // Calculate initial Rectangle (m_Bounds) immediately after creation
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

    const Color fillColor = isHovered ? NookCol::UI_PANEL_HOVER : NookCol::UI_PANEL;
    const Color borderColor = isHovered ? NookCol::UI_ACCENT : NookCol::UI_BORDER_SOFT;

    DrawRectangleRounded(m_Bounds, 0.22f, 10, fillColor);
    DrawRectangleRoundedLinesEx(m_Bounds, 0.22f, 10, 2.0f, borderColor);

    
    if (renderer) {
        Vector2 center = { m_Bounds.x + m_Bounds.width / 2.0f, m_Bounds.y + m_Bounds.height / 2.0f };
        renderer->DrawTextCentered(text, center, 19.0f, NookCol::UI_TEXT);
    }
}