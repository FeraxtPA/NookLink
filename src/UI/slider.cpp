#include "slider.h"
#include <raylib.h>
#include <cmath>

Slider::Slider(Anchor anchor, Vector2 offset, Vector2 size, float minV, float maxV, float initialVal, std::function<void(float)> onChangeCallback)
    : Widget(anchor, offset, size), minVal(minV), maxVal(maxV), value(initialVal), onChange(onChangeCallback)
{
    OnWindowResize(GetScreenWidth(), GetScreenHeight());
}

void Slider::Update() {
    if (!isVisible) return;

    Vector2 mousePos = GetMousePosition();

    // Detekce najetí myší
    if (CheckCollisionPointRec(mousePos, m_Bounds)) {
        isHovered = true;
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);

        // Zaèátek tažení
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            isDragging = true;
        }
    }
    else {
        isHovered = false;
    }

    // Konec tažení (kdekoli na obrazovce)
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        isDragging = false;
    }

    // Samotný pohyb sliderem
    if (isDragging) {
        // Zjistíme, jak daleko je myš od levého okraje slideru (0.0 až 1.0)
        float normalizedX = (mousePos.x - m_Bounds.x) / m_Bounds.width;

        // Omezíme meze, aby uživatel nevyjel mimo slider
        if (normalizedX < 0.0f) normalizedX = 0.0f;
        if (normalizedX > 1.0f) normalizedX = 1.0f;

        // Pøepoèet na reálnou hodnotu (napø. 0 až 5)
        float newVal = minVal + normalizedX * (maxVal - minVal);

        // Zaokrouhlení na jedno desetinné místo (napø. 4.3)
        newVal = std::round(newVal * 10.0f) / 10.0f;

        // Pokud se hodnota zmìnila, aktualizujeme ji a zavoláme callback
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

    // 1. Vykreslení prázdného pozadí (koleje)
    DrawRectangleRec(m_Bounds, LIGHTGRAY);

    // 2. Vykreslení vyplnìné èásti podle aktuální hodnoty
    float normalizedValue = (value - minVal) / (maxVal - minVal);
    Rectangle fillRec = { m_Bounds.x, m_Bounds.y, m_Bounds.width * normalizedValue, m_Bounds.height };
    DrawRectangleRec(fillRec, DARKGRAY);

    // 3. Vykreslení rámeèku okolo
    Color borderColor = isHovered || isDragging ? BLACK : GRAY;
    DrawRectangleLinesEx(m_Bounds, 2, borderColor);
}