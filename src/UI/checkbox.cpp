
// Implementation of the Checkbox widget class.
// Provides toggle functionality with visual feedback.


#include "checkbox.h"
#include "../textRenderer.h"
#include "../colors.h"
#include "../constants.h"


Checkbox::Checkbox(Anchor anchor, Vector2 offset, Vector2 size, std::string l, bool initial, std::function<void(bool)> onChangeCallback)
    : Widget(anchor, offset, size), label(l), checked(initial), onChange(onChangeCallback)
{
    OnWindowResize(GetScreenWidth(), GetScreenHeight());
}

void Checkbox::Update() {
    if (!m_IsVisible) return;

    Rectangle clickArea = m_Bounds;
    clickArea.width += NookConst::WidgetStyle::kCheckboxClickLabelExtension; // Allow clicking the text too

    const bool hovered = HandleHoverCursor(clickArea);
    const bool clicked = hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !IsLeftClickConsumed();
    if (clicked && GetTime() - m_LastClickTime > NookConst::Input::kUiClickDebounceSeconds) {
        ConsumeLeftClick();
        checked = !checked;
        if (onChange) onChange(checked);

        m_LastClickTime = GetTime();
    }
}

void Checkbox::Draw(TextRenderer* renderer) {
    if (!m_IsVisible) return;

    
    DrawRectangleRounded(m_Bounds, NookConst::WidgetStyle::kCheckboxRoundness, NookConst::WidgetStyle::kCheckboxRoundSegments, NookCol::UI_PANEL_ALT);

  
    Color borderColor = m_IsHovered ? NookCol::UI_BORDER : NookCol::UI_BORDER_SOFT;
    DrawRectangleRoundedLinesEx(m_Bounds, NookConst::WidgetStyle::kCheckboxRoundness, NookConst::WidgetStyle::kCheckboxRoundSegments, NookConst::WidgetStyle::kCheckboxBorderThickness, borderColor);

  
    if (checked) {
        const int inset = (int)NookConst::WidgetStyle::kCheckboxInnerInset;
        DrawRectangle((int)m_Bounds.x + inset, (int)m_Bounds.y + inset, (int)m_Bounds.width - inset * 2, (int)m_Bounds.height - inset * 2, NookCol::UI_ACCENT_SOFT);
    }

  
    if (renderer) {
        renderer->DrawSimpleText(label, { m_Bounds.x + m_Bounds.width + NookConst::WidgetStyle::kCheckboxLabelGap, m_Bounds.y }, NookConst::WidgetStyle::kCheckboxLabelFontSize, NookCol::UI_TEXT);
    }
}