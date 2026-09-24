
// Implementation of the Button widget class.
// Provides clickable button with hover states and callbacks.


#include "button.h"
#include "../textRenderer.h"
#include "../iconRenderer.h"
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


    if (renderer && m_ContentType == ContentType::Text) {
        Vector2 center = { m_Bounds.x + m_Bounds.width / 2.0f, m_Bounds.y + m_Bounds.height / 2.0f };
        renderer->DrawTextCentered(m_Text, center, NookConst::WidgetStyle::kButtonFontSize, NookCol::UI_TEXT);
    }
}

void Button::DrawWithIcons(TextRenderer* renderer, IconRenderer* iconRenderer) {
    if (!m_IsVisible) return;

    const Color fillColor = m_IsHovered ? NookCol::UI_PANEL_HOVER : NookCol::UI_PANEL;
    const Color borderColor = m_IsHovered ? NookCol::UI_ACCENT : NookCol::UI_BORDER_SOFT;

    DrawRectangleRounded(m_Bounds, NookConst::WidgetStyle::kButtonRoundness, NookConst::WidgetStyle::kButtonRoundSegments, fillColor);
    DrawRectangleRoundedLinesEx(m_Bounds, NookConst::WidgetStyle::kButtonRoundness, NookConst::WidgetStyle::kButtonRoundSegments, NookConst::WidgetStyle::kButtonBorderThickness, borderColor);

    const Color contentColor = NookCol::UI_TEXT;
    Vector2 center = { m_Bounds.x + m_Bounds.width / 2.0f, m_Bounds.y + m_Bounds.height / 2.0f };

    if (m_ContentType == ContentType::Icon && iconRenderer && m_IconType >= 0) {
        const float iconSize = 20.0f;
        const float gap = 8.0f;

        if (!m_Text.empty() && renderer) {
            const float textWidth = renderer->Measure(m_Text, NookConst::WidgetStyle::kButtonFontSize);
            const float groupWidth = iconSize + gap + textWidth;
            const float startX = center.x - (groupWidth * 0.5f);

            const Vector2 iconCenter = { startX + (iconSize * 0.5f), center.y };
            const Vector2 textCenter = { startX + iconSize + gap + (textWidth * 0.5f), center.y };

            iconRenderer->DrawIconCentered(static_cast<IconRenderer::IconType>(m_IconType), iconCenter, iconSize, contentColor);
            renderer->DrawTextCentered(m_Text, textCenter, NookConst::WidgetStyle::kButtonFontSize, contentColor);
            return;
        }

        iconRenderer->DrawIconCentered(static_cast<IconRenderer::IconType>(m_IconType), center, 24.0f, contentColor);
    } else if (m_ContentType == ContentType::Text && renderer) {
        renderer->DrawTextCentered(m_Text, center, NookConst::WidgetStyle::kButtonFontSize, contentColor);
    }
}