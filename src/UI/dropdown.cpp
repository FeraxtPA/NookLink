// Dropdown widget implementation.
// Draws a selectable list and handles pointer interaction.

#include "dropdown.h"

#include "../colors.h"
#include "../constants.h"
#include "../textRenderer.h"

#include <algorithm>

namespace {
	// Helper to calculate the rectangle for the expanded options list based on the base dropdown rectangle and number of visible options.
Rectangle GetExpandedRect(const Rectangle& baseRect, int visibleCount) {
    return Rectangle{
        baseRect.x,
        baseRect.y + baseRect.height,
        baseRect.width,
        baseRect.height * (float)visibleCount,
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

	// Notify callback if requested and valid.
    if (notify && m_OnSelect) {
        m_OnSelect(m_SelectedIndex, m_Options[(size_t)m_SelectedIndex]);
    }
}

void Dropdown::SetMaxVisibleOptions(int maxVisible) {
    m_MaxVisibleOptions = std::max(1, maxVisible);
}

std::string Dropdown::GetSelectedText() const {
    if (m_Options.empty()) return "";
    return m_Options[(size_t)m_SelectedIndex];
}

void Dropdown::Update() {
    if (!m_IsVisible) return;

    const Vector2 mouse = GetMousePosition();
    const bool hoveredBase = HandleHoverCursor(m_Bounds);
    const int optionCount = (int)m_Options.size();
    const int visibleCount = std::min(std::max(1, m_MaxVisibleOptions), std::max(0, optionCount));
    const int maxScrollOffset = std::max(0, optionCount - visibleCount);
    const Rectangle expandedRect = GetExpandedRect(m_Bounds, visibleCount);
    const bool hoveredExpandedRect = m_IsExpanded && visibleCount > 0 && CheckCollisionPointRec(mouse, expandedRect);

    m_HoveredOptionIndex = -1;
    if (m_IsExpanded && visibleCount > 0) {
        const float wheel = GetMouseWheelMove();

		// Scroll options if mouse is over the expanded list and wheel is moved.
        if (hoveredExpandedRect && wheel != 0.0f) {
            if (wheel > 0.0f) m_ScrollOffset -= 1;
            else m_ScrollOffset += 1;
            m_ScrollOffset = std::clamp(m_ScrollOffset, 0, maxScrollOffset);
        }

		// Check which option is hovered by iterating through visible options and checking mouse collision.
        for (int row = 0; row < visibleCount; ++row) {
            const int optionIndex = m_ScrollOffset + row;
            if (optionIndex < 0 || optionIndex >= optionCount) {
                continue;
            }

            const Rectangle optionRect = Rectangle{
                m_Bounds.x,
                expandedRect.y + m_Bounds.height * (float)row,
                m_Bounds.width,
                m_Bounds.height,
            };

            if (CheckCollisionPointRec(mouse, optionRect)) {
                m_HoveredOptionIndex = optionIndex;
                break;
            }
        }
    }

    if (hoveredExpandedRect || m_HoveredOptionIndex >= 0) {
        RequestCursor(MOUSE_CURSOR_POINTING_HAND);
    }

    // Toggle expansion if base is clicked.
    if (ConsumeLeftClickOnHover(m_Bounds)) {
        m_IsExpanded = !m_IsExpanded;
        return;
    }

    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || Widget::IsLeftClickConsumed()) {
        return;
    }
	// If an option is clicked while expanded, select it and collapse.
    if (m_IsExpanded && m_HoveredOptionIndex >= 0) {
        SetSelectedIndex(m_HoveredOptionIndex, true);
        m_IsExpanded = false;
        Widget::ConsumeLeftClick();
        return;
    }

	// If click outside base but inside expanded area, consume click to prevent closing.
    if (m_IsExpanded && hoveredExpandedRect) {
        Widget::ConsumeLeftClick();
        return;
    }

	// If click outside both base and expanded area, close dropdown.
    if (m_IsExpanded) {
        m_IsExpanded = false;
        Widget::ConsumeLeftClick();
    }
}

void Dropdown::Draw(TextRenderer* renderer) {
    if (!m_IsVisible || !renderer) return;

    const Color baseColor = m_IsHovered ? NookCol::UI_PANEL_HOVER : NookCol::UI_PANEL_ALT;

	// Draw base rectangle with rounded corners and border.
    DrawRectangleRounded(m_Bounds, NookConst::WidgetStyle::kDropdownBaseRoundness, NookConst::WidgetStyle::kDropdownBaseRoundSegments, baseColor);
    DrawRectangleRoundedLinesEx(m_Bounds, NookConst::WidgetStyle::kDropdownBaseRoundness, NookConst::WidgetStyle::kDropdownBaseRoundSegments, NookConst::WidgetStyle::kDropdownBorderThickness, NookCol::UI_BORDER_SOFT);

    const std::string selectedText = GetSelectedText();

    renderer->DrawSimpleText(selectedText.empty() ? "Select..." : selectedText,
        { m_Bounds.x + NookConst::WidgetStyle::kDropdownTextPaddingX, m_Bounds.y + NookConst::WidgetStyle::kDropdownTextPaddingY },
        NookConst::WidgetStyle::kDropdownFontSize,
        selectedText.empty() ? NookCol::UI_TEXT_MUTED : NookCol::UI_TEXT);


	// Draw expansion indicator (arrow) on the right side of the base rectangle.
    renderer->DrawSimpleText(m_IsExpanded ? "^" : "v",
        { m_Bounds.x + m_Bounds.width - NookConst::WidgetStyle::kDropdownArrowRightInset, m_Bounds.y + NookConst::WidgetStyle::kDropdownArrowTopInset },
        NookConst::WidgetStyle::kDropdownFontSize,
        NookCol::UI_TEXT_MUTED);

    if (!m_IsExpanded) {
        return;
    }

    const int optionCount = (int)m_Options.size();
    if (optionCount <= 0) {
        return;
    }

    const int visibleCount = std::min(std::max(1, m_MaxVisibleOptions), optionCount);
    const int maxScrollOffset = std::max(0, optionCount - visibleCount);
    m_ScrollOffset = std::clamp(m_ScrollOffset, 0, maxScrollOffset);

    const Rectangle expandedRect = GetExpandedRect(m_Bounds, visibleCount);

	// Draw expanded options background and border.
    DrawRectangleRec(expandedRect, NookCol::UI_PANEL);
    DrawRectangleLinesEx(expandedRect, NookConst::WidgetStyle::kDropdownOptionBorderThickness, NookCol::UI_BORDER_SOFT);

	// Set scissor mode to expandedRect to ensure options are clipped within it.
    BeginScissorMode((int)expandedRect.x, (int)expandedRect.y, (int)expandedRect.width, (int)expandedRect.height);

    for (int row = 0; row < visibleCount; ++row) {
        const int optionIndex = m_ScrollOffset + row;
        if (optionIndex < 0 || optionIndex >= optionCount) {
            continue;
        }

        const Rectangle optionRect = Rectangle{
            m_Bounds.x,
            expandedRect.y + m_Bounds.height * (float)row,
            m_Bounds.width,
            m_Bounds.height,
        };

        const bool isSelected = (optionIndex == m_SelectedIndex);
        const bool isHoveredOption = (optionIndex == m_HoveredOptionIndex);

        Color optionColor = NookCol::UI_PANEL;
        if (isSelected) optionColor = NookCol::UI_PANEL_ALT;
        if (isHoveredOption) optionColor = NookCol::UI_PANEL_HOVER;

        DrawRectangleRec(optionRect, optionColor);
        DrawRectangleLinesEx(optionRect, NookConst::WidgetStyle::kDropdownOptionBorderThickness, NookCol::UI_BORDER_SOFT);

		// Draw option text on expanded list
        renderer->DrawSimpleText(
            m_Options[(size_t)optionIndex],
            { optionRect.x + NookConst::WidgetStyle::kDropdownTextPaddingX, optionRect.y + NookConst::WidgetStyle::kDropdownTextPaddingY },
            NookConst::WidgetStyle::kDropdownFontSize,
            NookCol::UI_TEXT);
            
    }

    EndScissorMode();

	// If there are more options than visible, draw a scrollbar on the right side of the expanded area.
    if (optionCount > visibleCount) {
        const float trackWidth = NookConst::WidgetStyle::kDropdownScrollbarTrackWidth;
        const float trackPadding = NookConst::WidgetStyle::kDropdownScrollbarTrackPadding;
        const float trackX = expandedRect.x + expandedRect.width - trackWidth - trackPadding;
        const float trackY = expandedRect.y + trackPadding;
        const float trackHeight = expandedRect.height - trackPadding * 2.0f;

        const float thumbHeight = std::max(NookConst::WidgetStyle::kDropdownScrollbarThumbMinHeight, trackHeight * ((float)visibleCount / (float)optionCount));
        const float t = maxScrollOffset > 0 ? ((float)m_ScrollOffset / (float)maxScrollOffset) : 0.0f;
        const float thumbY = trackY + (trackHeight - thumbHeight) * t;

        DrawRectangleRounded({ trackX, trackY, trackWidth, trackHeight }, NookConst::WidgetStyle::kDropdownScrollbarRoundness, NookConst::WidgetStyle::kDropdownScrollbarRoundSegments, Fade(NookCol::UI_BORDER_SOFT, 0.55f));
        DrawRectangleRounded({ trackX, thumbY, trackWidth, thumbHeight }, NookConst::WidgetStyle::kDropdownScrollbarRoundness, NookConst::WidgetStyle::kDropdownScrollbarRoundSegments, NookCol::UI_ACCENT_SOFT);
    }
}
