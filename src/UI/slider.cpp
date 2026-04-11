
// Implementation of the Slider widget class.
// Provides smooth value dragging and interactive slider bar.


#include "slider.h"
#include <raylib.h>
#include <cmath>
#include "../colors.h"
#include "../constants.h"

Slider::Slider(Anchor anchor, Vector2 offset, Vector2 size, float minV, float maxV, float initialVal, std::function<void(float)> onChangeCallback)
    : Widget(anchor, offset, size), minVal(minV), maxVal(maxV), value(initialVal), onChange(onChangeCallback)
{
    OnWindowResize(GetScreenWidth(), GetScreenHeight());
}

void Slider::Update() {
    if (!m_IsVisible) return;

    Vector2 mousePos = GetMousePosition();

    if (ConsumeLeftClickOnHover(m_Bounds)) {
        isDragging = true;
    }
    else if (!isDragging) {
        HandleHoverCursor(m_Bounds);
    }

   
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        isDragging = false;
    }

    
    if (isDragging) {
        // Map cursor X within slider bounds to normalized [0, 1] range.
        float normalizedX = (mousePos.x - m_Bounds.x) / m_Bounds.width;

        
        if (normalizedX < 0.0f) normalizedX = 0.0f;
        if (normalizedX > 1.0f) normalizedX = 1.0f;

        // Convert normalized position to value domain [minVal, maxVal].
        float newVal = minVal + normalizedX * (maxVal - minVal);

        // Keep one decimal place for stable UI labels and filtering logic.
        newVal = std::round(newVal * NookConst::WidgetStyle::kSliderStepScale) / NookConst::WidgetStyle::kSliderStepScale;

     
        if (newVal != value) {
            value = newVal;
            if (onChange) {
                onChange(value);
            }
        }
    }
}

void Slider::Draw(TextRenderer* renderer) {
    if (!m_IsVisible) return;

   
    DrawRectangleRounded(m_Bounds, NookConst::WidgetStyle::kSliderRoundness, NookConst::WidgetStyle::kSliderRoundSegments, NookCol::UI_PANEL_ALT);

    // Draw filled progress segment proportional to current value.
    float normalizedValue = (value - minVal) / (maxVal - minVal);
    Rectangle fillRec = { m_Bounds.x, m_Bounds.y, m_Bounds.width * normalizedValue, m_Bounds.height };
    DrawRectangleRounded(fillRec, NookConst::WidgetStyle::kSliderRoundness, NookConst::WidgetStyle::kSliderRoundSegments, NookCol::UI_ACCENT_SOFT);

  
    Color borderColor = m_IsHovered || isDragging ? NookCol::UI_ACCENT : NookCol::UI_BORDER_SOFT;
    DrawRectangleLinesEx(m_Bounds, NookConst::WidgetStyle::kSliderBorderThickness, borderColor);
}