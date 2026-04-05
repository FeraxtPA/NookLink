#include "checkbox.h"
#include "../textRenderer.h"

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

            // Pokud od posledního pøepnutí ubìhlo více než 0.2 sekundy
            if (GetTime() - m_LastClickTime > 0.2) {
                checked = !checked;                  // Pøepneme stav
                if (onChange) onChange(checked); // Zavoláme akci

                m_LastClickTime = GetTime();         // Zapíšeme si èas
            }
        }
    }
    else {
        isHovered = false;
    }
}

void Checkbox::Draw(TextRenderer* renderer) {
    if (!isVisible) return;

    
    DrawRectangleRec(m_Bounds, RAYWHITE);

  
    Color borderColor = isHovered ? DARKGRAY : LIGHTGRAY;
    DrawRectangleLinesEx(m_Bounds, 2, borderColor);

  
    if (checked) {
        DrawRectangle((int)m_Bounds.x + 4, (int)m_Bounds.y + 4, (int)m_Bounds.width - 8, (int)m_Bounds.height - 8, DARKGRAY);
    }

  
    if (renderer) {
        renderer->DrawSimpleText(label, { m_Bounds.x + m_Bounds.width + 10, m_Bounds.y }, 20, BLACK);
    }
}