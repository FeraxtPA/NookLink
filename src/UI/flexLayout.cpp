
// Implementation of the FlexLayout widget class.
// Manages child widget positioning with flexible and fixed sizing.


#include "flexLayout.h"
#include "dropdown.h"

#include <algorithm>
#include <vector>

FlexLayout::FlexLayout(Anchor anchor, Vector2 offset, Vector2 size, Direction direction, Vector2 padding, float gap, CrossAlign crossAlign)
    : Widget(anchor, offset, size), m_Direction(direction), m_CrossAlign(crossAlign), m_Padding(padding), m_Gap(gap)
{
    OnWindowResize(GetScreenWidth(), GetScreenHeight());
}

void FlexLayout::AddChild(std::shared_ptr<Widget> widget, Vector2 size, float grow)
{
    m_Items.push_back({ widget, size, grow });
    LayoutChildren();
}

// Add a spacer item that takes up flexible space without rendering anything.
void FlexLayout::AddSpacer(float grow)
{
    m_Items.push_back({ nullptr, { 0.0f, 0.0f }, grow });
    LayoutChildren();
}


// Remove all children and reset layout.
void FlexLayout::ClearChildren()
{
    m_Items.clear();
    LayoutChildren();
}

// Override to trigger child layout when the window is resized.
void FlexLayout::OnWindowResize(int screenWidth, int screenHeight)
{
    Widget::OnWindowResize(screenWidth, screenHeight);
    LayoutChildren();
}

void FlexLayout::BeginFrameInputRecursive()
{
    BeginFrameInput();

    for (auto& item : m_Items) {
        if (item.widget) {
            item.widget->BeginFrameInputRecursive();
        }
    }
}

MouseCursor FlexLayout::ResolveRequestedCursorRecursive() const
{
    if (!m_IsVisible) {
        return MOUSE_CURSOR_DEFAULT;
    }

    for (auto it = m_Items.rbegin(); it != m_Items.rend(); ++it) {
        if (!it->widget) continue;
        const MouseCursor requested = it->widget->ResolveRequestedCursorRecursive();
        if (requested != MOUSE_CURSOR_DEFAULT) {
            return requested;
        }
    }

    return GetRequestedCursor();
}

void FlexLayout::Update()
{
    if (!m_IsVisible) return;

    LayoutChildren();

    for (auto& item : m_Items) {
        if (item.widget) {
            item.widget->Update();
        }
    }
}

void FlexLayout::Draw(TextRenderer* renderer)
{
    if (!m_IsVisible) return;

    LayoutChildren();

    std::vector<std::shared_ptr<Dropdown>> expandedDropdowns;
    expandedDropdowns.reserve(m_Items.size());

	// First draw all non-dropdown widgets and track expanded dropdowns to draw them last.
    for (auto& item : m_Items) {
        if (item.widget) {
            auto dropdown = std::dynamic_pointer_cast<Dropdown>(item.widget);
            if (dropdown && dropdown->IsExpanded()) {
                expandedDropdowns.push_back(dropdown);
                continue;
            }

            item.widget->Draw(renderer);
        }
    }

    // Draw expanded dropdowns last so option menus stay above neighboring fields.
    for (auto& dropdown : expandedDropdowns) {
        dropdown->Draw(renderer);
    }
}

void FlexLayout::LayoutChildren()
{
    if (m_Items.empty()) return;

	// Calculate content area by applying padding to the layout's bounds.
    const float contentX = m_Bounds.x + m_Padding.x;
    const float contentY = m_Bounds.y + m_Padding.y;
    const float contentWidth = std::max(0.0f, m_Bounds.width - (m_Padding.x * 2.0f));
    const float contentHeight = std::max(0.0f, m_Bounds.height - (m_Padding.y * 2.0f));

    if (m_Direction == Direction::Horizontal) {
        float fixedWidth = 0.0f;
        float growTotal = 0.0f;
        int visibleCount = 0;

        for (const auto& item : m_Items) {
            if (item.widget || item.grow > 0.0f) {
                ++visibleCount;
            }

            if (item.grow > 0.0f) {
                growTotal += item.grow;
            }
            else {
                fixedWidth += item.size.x;
            }
        }

        // Free space = content - fixed items - inter-item gaps.
        const float totalGap = m_Gap * std::max(0, visibleCount - 1);
        float remaining = contentWidth - fixedWidth - totalGap;
        if (remaining < 0.0f) remaining = 0.0f;

        // Each grow unit receives proportional share of remaining width.
        const float growUnit = growTotal > 0.0f ? remaining / growTotal : 0.0f;
        float currentX = contentX;

        for (const auto& item : m_Items) {
            const float itemWidth = (item.grow > 0.0f) ? (growUnit * item.grow) : item.size.x;
            const float itemHeight = (m_CrossAlign == CrossAlign::Stretch) ? contentHeight : (item.size.y > 0.0f ? item.size.y : contentHeight);

            if (item.widget) {
                float y = contentY;
                // Cross-axis alignment for horizontal flow affects Y only.
                switch (m_CrossAlign) {
                case CrossAlign::Start:   y = contentY; break;
                case CrossAlign::Center:  y = contentY + (contentHeight - itemHeight) * 0.5f; break;
                case CrossAlign::End:     y = contentY + contentHeight - itemHeight; break;
                case CrossAlign::Stretch: y = contentY; break;
                }

                item.widget->SetBounds({ currentX, y, itemWidth, itemHeight });
            }

            currentX += itemWidth + m_Gap;
        }
        return;
    }

    float fixedHeight = 0.0f;
    float growTotal = 0.0f;
    int visibleCount = 0;

    for (const auto& item : m_Items) {
        if (item.widget || item.grow > 0.0f) {
            ++visibleCount;
        }

        if (item.grow > 0.0f) {
            growTotal += item.grow;
        }
        else {
            fixedHeight += item.size.y;
        }
    }

    // Vertical flow mirrors horizontal logic, but distributes remaining height.
    const float totalGap = m_Gap * std::max(0, visibleCount - 1);
    float remaining = contentHeight - fixedHeight - totalGap;
    if (remaining < 0.0f) remaining = 0.0f;

    const float growUnit = growTotal > 0.0f ? remaining / growTotal : 0.0f;
    float currentY = contentY;

	// Layout each item vertically, applying cross-axis alignment for horizontal positioning.
    for (const auto& item : m_Items) {
        const float itemHeight = (item.grow > 0.0f) ? (growUnit * item.grow) : item.size.y;
        const float itemWidth = (m_CrossAlign == CrossAlign::Stretch) ? contentWidth : (item.size.x > 0.0f ? item.size.x : contentWidth);

        if (item.widget) {
            float x = contentX;
            // Cross-axis alignment for vertical flow affects X only.
            switch (m_CrossAlign) {
            case CrossAlign::Start:   x = contentX; break;
            case CrossAlign::Center:  x = contentX + (contentWidth - itemWidth) * 0.5f; break;
            case CrossAlign::End:     x = contentX + contentWidth - itemWidth; break;
            case CrossAlign::Stretch: x = contentX; break;
            }

            item.widget->SetBounds({ x, currentY, itemWidth, itemHeight });
        }

        currentY += itemHeight + m_Gap;
    }
}