
// Implementation of the Checkbox widget class.
// Provides toggle functionality with visual feedback.


#include "checkbox.h"
#include "../textRenderer.h"
#include "../colors.h"

Checkbox::Checkbox(Anchor anchor, Vector2 offset, Vector2 size, std::string l, bool initial, std::function<void(bool)> onChangeCallback)
    : Widget(anchor, offset, size), label(l), checked(initial), onChange(onChangeCallback)
{
    OnWindowResize(GetScreenWidth(), GetScreenHeight());
}

void Checkbox::Update() {
    if (!isVisible) return;

    
    Rectangle clickArea = m_Bounds;
    clickArea.width += 200; // Allow clicking the text too

    if (CheckCollisionPointRec(GetMousePosition(), clickArea)) {
        isHovered = true;
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {

            
            if (GetTime() - m_LastClickTime > 0.2) {
                checked = !checked;                  
                if (onChange) onChange(checked); 

                m_LastClickTime = GetTime();        
            }
        }
    }
    else {
        isHovered = false;
    }
}

void Checkbox::Draw(TextRenderer* renderer) {
    if (!isVisible) return;

    
    DrawRectangleRounded(m_Bounds, 0.18f, 8, NookCol::UI_PANEL_ALT);

  
    Color borderColor = isHovered ? NookCol::UI_BORDER : NookCol::UI_BORDER_SOFT;
    DrawRectangleRoundedLinesEx(m_Bounds, 0.18f, 8, 2.0f, borderColor);

  
    if (checked) {
        DrawRectangle((int)m_Bounds.x + 4, (int)m_Bounds.y + 4, (int)m_Bounds.width - 8, (int)m_Bounds.height - 8, NookCol::UI_ACCENT_SOFT);
    }

  
    if (renderer) {
        renderer->DrawSimpleText(label, { m_Bounds.x + m_Bounds.width + 10, m_Bounds.y }, 20, NookCol::UI_TEXT);
    }
}