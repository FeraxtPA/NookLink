
// Implementation of the Button widget class.
// Provides clickable button with hover states and callbacks.


#include "button.h"
#include "../textRenderer.h" 
#include "../colors.h"
#include "../constants.h"

Button::Button(Anchor anchor, Vector2 offset, Vector2 size, std::string t, std::function<void()> callback)
    : Widget(anchor, offset, size), m_Text(t), m_OnClick(callback)
{
    // Calculate initial Rectangle (m_Bounds) immediately after creation
    OnWindowResize(GetScreenWidth(), GetScreenHeight());
}

void Button::Update() {
    if (!m_IsVisible) return;

    if (ConsumeLeftClickOnHover(m_Bounds) && m_OnClick) {
        m_OnClick();
    }
}

void Button::Draw(TextRenderer* renderer) {
    if (!m_IsVisible) return;

    const Color fillColor = m_IsHovered ? NookCol::UI_PANEL_HOVER : NookCol::UI_PANEL;
    const Color borderColor = m_IsHovered ? NookCol::UI_ACCENT : NookCol::UI_BORDER_SOFT;

    DrawRectangleRounded(m_Bounds, NookConst::WidgetStyle::kButtonRoundness, NookConst::WidgetStyle::kButtonRoundSegments, fillColor);
    DrawRectangleRoundedLinesEx(m_Bounds, NookConst::WidgetStyle::kButtonRoundness, NookConst::WidgetStyle::kButtonRoundSegments, NookConst::WidgetStyle::kButtonBorderThickness, borderColor);

    
    if (renderer) {
        Vector2 center = { m_Bounds.x + m_Bounds.width / 2.0f, m_Bounds.y + m_Bounds.height / 2.0f };
        renderer->DrawTextCentered(m_Text, center, NookConst::WidgetStyle::kButtonFontSize, NookCol::UI_TEXT);
    }
}