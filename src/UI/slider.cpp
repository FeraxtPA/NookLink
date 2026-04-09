#include "slider.h"
#include <raylib.h>
#include <cmath>
#include "../colors.h"

Slider::Slider(Anchor anchor, Vector2 offset, Vector2 size, float minV, float maxV, float initialVal, std::function<void(float)> onChangeCallback)
    : Widget(anchor, offset, size), minVal(minV), maxVal(maxV), value(initialVal), onChange(onChangeCallback)
{
    OnWindowResize(GetScreenWidth(), GetScreenHeight());
}

void Slider::Update() {
    if (!isVisible) return;

    Vector2 mousePos = GetMousePosition();

    // Detekce najet� my��
    if (CheckCollisionPointRec(mousePos, m_Bounds)) {
        isHovered = true;
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);

        // Za��tek ta�en�
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            isDragging = true;
        }
    }
    else {
        isHovered = false;
    }

    // Konec ta�en� (kdekoli na obrazovce)
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        isDragging = false;
    }

    // Samotn� pohyb sliderem
    if (isDragging) {
        // Zjist�me, jak daleko je my� od lev�ho okraje slideru (0.0 a� 1.0)
        float normalizedX = (mousePos.x - m_Bounds.x) / m_Bounds.width;

        // Omez�me meze, aby u�ivatel nevyjel mimo slider
        if (normalizedX < 0.0f) normalizedX = 0.0f;
        if (normalizedX > 1.0f) normalizedX = 1.0f;

        // P�epo�et na re�lnou hodnotu (nap�. 0 a� 5)
        float newVal = minVal + normalizedX * (maxVal - minVal);

        // Zaokrouhlen� na jedno desetinn� m�sto (nap�. 4.3)
        newVal = std::round(newVal * 10.0f) / 10.0f;

        // Pokud se hodnota zm�nila, aktualizujeme ji a zavol�me callback
        if (newVal != value) {
            value = newVal;
            if (onChange) {
                onChange(value);
            }
        }
    }
}

void Slider::Draw(TextRenderer* renderer) {
    if (!isVisible) return;

    // 1. Vykreslen� pr�zdn�ho pozad� (koleje)
    DrawRectangleRounded(m_Bounds, 0.5f, 12, NookCol::UI_PANEL_ALT);

    // 2. Vykreslen� vypln�n� ��sti podle aktu�ln� hodnoty
    float normalizedValue = (value - minVal) / (maxVal - minVal);
    Rectangle fillRec = { m_Bounds.x, m_Bounds.y, m_Bounds.width * normalizedValue, m_Bounds.height };
    DrawRectangleRounded(fillRec, 0.5f, 12, NookCol::UI_ACCENT_SOFT);

    // 3. Vykreslen� r�me�ku okolo
    Color borderColor = isHovered || isDragging ? NookCol::UI_ACCENT : NookCol::UI_BORDER_SOFT;
    DrawRectangleLinesEx(m_Bounds, 2, borderColor);
}