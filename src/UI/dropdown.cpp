// Dropdown widget implementation.
// Draws a selectable list and handles pointer interaction.

#include "dropdown.h"

#include "../colors.h"
#include "../textRenderer.h"

#include <algorithm>

namespace {
Rectangle GetOptionRect(const Rectangle& baseRect, int optionIndex) {
    return Rectangle{
        baseRect.x,
        baseRect.y + baseRect.height * (float)(optionIndex + 1),
        baseRect.width,
        baseRect.height,
    };
}
}

Dropdown::Dropdown(
    Anchor anchor,
    Vector2 offset,
    Vector2 size,
    std::vector<std::string> options,
    int initialIndex,
    std::function<void(int, const std::string&)> onSelect)
    : Widget(anchor, offset, size),
      m_Options(std::move(options)),
      m_OnSelect(std::move(onSelect)) {
    SetSelectedIndex(initialIndex, false);
    OnWindowResize(GetScreenWidth(), GetScreenHeight());
}

void Dropdown::SetOptions(const std::vector<std::string>& options, int selectedIndex) {
    m_Options = options;
    SetSelectedIndex(selectedIndex, false);
}

void Dropdown::SetSelectedIndex(int index, bool notify) {
    if (m_Options.empty()) {
        m_SelectedIndex = 0;
        return;
    }

    m_SelectedIndex = std::clamp(index, 0, (int)m_Options.size() - 1);
    if (notify && m_OnSelect) {
        m_OnSelect(m_SelectedIndex, m_Options[(size_t)m_SelectedIndex]);
    }
}

std::string Dropdown::GetSelectedText() const {
    if (m_Options.empty()) return "";
    return m_Options[(size_t)m_SelectedIndex];
}

void Dropdown::Update() {
    if (!isVisible) return;

    const Vector2 mouse = GetMousePosition();
    const bool hoveredBase = CheckCollisionPointRec(mouse, m_Bounds);
    isHovered = hoveredBase;

    m_HoveredOptionIndex = -1;
    if (m_IsExpanded) {
        for (int i = 0; i < (int)m_Options.size(); ++i) {
            if (CheckCollisionPointRec(mouse, GetOptionRect(m_Bounds, i))) {
                m_HoveredOptionIndex = i;
                break;
            }
        }
    }

    if (hoveredBase || m_HoveredOptionIndex >= 0) {
        Widget::DesiredCursor = MOUSE_CURSOR_POINTING_HAND;
    }

    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        return;
    }

    if (hoveredBase) {
        m_IsExpanded = !m_IsExpanded;
        return;
    }

    if (m_IsExpanded && m_HoveredOptionIndex >= 0) {
        SetSelectedIndex(m_HoveredOptionIndex, true);
        m_IsExpanded = false;
        return;
    }

    if (m_IsExpanded) {
        m_IsExpanded = false;
    }
}

void Dropdown::Draw(TextRenderer* renderer) {
    if (!isVisible || !renderer) return;

    const Color baseColor = isHovered ? NookCol::UI_PANEL_HOVER : NookCol::UI_PANEL_ALT;
    DrawRectangleRounded(m_Bounds, 0.2f, 8, baseColor);
    DrawRectangleRoundedLinesEx(m_Bounds, 0.2f, 8, 2.0f, NookCol::UI_BORDER_SOFT);

    const std::string selectedText = GetSelectedText();
    renderer->DrawSimpleText(selectedText.empty() ? "Select..." : selectedText,
        { m_Bounds.x + 12.0f, m_Bounds.y + 9.0f },
        20,
        selectedText.empty() ? NookCol::UI_TEXT_MUTED : NookCol::UI_TEXT);

    renderer->DrawSimpleText(m_IsExpanded ? "^" : "v",
        { m_Bounds.x + m_Bounds.width - 22.0f, m_Bounds.y + 8.0f },
        20,
        NookCol::UI_TEXT_MUTED);

    if (!m_IsExpanded) {
        return;
    }

    for (int i = 0; i < (int)m_Options.size(); ++i) {
        const Rectangle optionRect = GetOptionRect(m_Bounds, i);
        const bool isSelected = (i == m_SelectedIndex);
        const bool isHoveredOption = (i == m_HoveredOptionIndex);

        Color optionColor = NookCol::UI_PANEL;
        if (isSelected) optionColor = NookCol::UI_PANEL_ALT;
        if (isHoveredOption) optionColor = NookCol::UI_PANEL_HOVER;

        DrawRectangleRec(optionRect, optionColor);
        DrawRectangleLinesEx(optionRect, 1.0f, NookCol::UI_BORDER_SOFT);

        renderer->DrawSimpleText(
            m_Options[(size_t)i],
            { optionRect.x + 12.0f, optionRect.y + 9.0f },
            20,
            NookCol::UI_TEXT);
    }
}
