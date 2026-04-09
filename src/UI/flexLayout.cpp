#include "flexLayout.h"

#include <algorithm>

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

void FlexLayout::AddSpacer(float grow)
{
    m_Items.push_back({ nullptr, { 0.0f, 0.0f }, grow });
    LayoutChildren();
}

void FlexLayout::ClearChildren()
{
    m_Items.clear();
    LayoutChildren();
}

void FlexLayout::OnWindowResize(int screenWidth, int screenHeight)
{
    Widget::OnWindowResize(screenWidth, screenHeight);
    LayoutChildren();
}

void FlexLayout::Update()
{
    if (!isVisible) return;

    LayoutChildren();

    for (auto& item : m_Items) {
        if (item.widget) {
            item.widget->Update();
        }
    }
}

void FlexLayout::Draw(TextRenderer* renderer)
{
    if (!isVisible) return;

    LayoutChildren();

    for (auto& item : m_Items) {
        if (item.widget) {
            item.widget->Draw(renderer);
        }
    }
}

void FlexLayout::LayoutChildren()
{
    if (m_Items.empty()) return;

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

        const float totalGap = m_Gap * std::max(0, visibleCount - 1);
        float remaining = contentWidth - fixedWidth - totalGap;
        if (remaining < 0.0f) remaining = 0.0f;

        const float growUnit = growTotal > 0.0f ? remaining / growTotal : 0.0f;
        float currentX = contentX;

        for (const auto& item : m_Items) {
            const float itemWidth = (item.grow > 0.0f) ? (growUnit * item.grow) : item.size.x;
            const float itemHeight = (m_CrossAlign == CrossAlign::Stretch) ? contentHeight : (item.size.y > 0.0f ? item.size.y : contentHeight);

            if (item.widget) {
                float y = contentY;
                switch (m_CrossAlign) {
                case CrossAlign::Start:   y = contentY; break;
                case CrossAlign::Center:  y = contentY + (contentHeight - itemHeight) * 0.5f; break;
                case CrossAlign::End:     y = contentY + contentHeight - itemHeight; break;
                case CrossAlign::Stretch: y = contentY; break;
                }

                item.widget->m_Bounds = { currentX, y, itemWidth, itemHeight };
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

    const float totalGap = m_Gap * std::max(0, visibleCount - 1);
    float remaining = contentHeight - fixedHeight - totalGap;
    if (remaining < 0.0f) remaining = 0.0f;

    const float growUnit = growTotal > 0.0f ? remaining / growTotal : 0.0f;
    float currentY = contentY;

    for (const auto& item : m_Items) {
        const float itemHeight = (item.grow > 0.0f) ? (growUnit * item.grow) : item.size.y;
        const float itemWidth = (m_CrossAlign == CrossAlign::Stretch) ? contentWidth : (item.size.x > 0.0f ? item.size.x : contentWidth);

        if (item.widget) {
            float x = contentX;
            switch (m_CrossAlign) {
            case CrossAlign::Start:   x = contentX; break;
            case CrossAlign::Center:  x = contentX + (contentWidth - itemWidth) * 0.5f; break;
            case CrossAlign::End:     x = contentX + contentWidth - itemWidth; break;
            case CrossAlign::Stretch: x = contentX; break;
            }

            item.widget->m_Bounds = { x, currentY, itemWidth, itemHeight };
        }

        currentY += itemHeight + m_Gap;
    }
}