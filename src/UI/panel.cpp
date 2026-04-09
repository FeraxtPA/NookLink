
// Implementation of the Panel widget class.
// Manages panel rendering with title bar and content layout.


#include "panel.h"
#include "../textRenderer.h"
#include "../colors.h"

#include <algorithm>

namespace {
constexpr float kPanelTitleBarHeight = 40.0f;
}

Panel::Panel(Anchor anchor, Vector2 offset, Vector2 size, std::string t)
    : Widget(anchor, offset, size), title(t)
{
    OnWindowResize(GetScreenWidth(), GetScreenHeight());
}

std::shared_ptr<FlexLayout> Panel::CreateContentLayout(
    FlexLayout::Direction direction,
    Vector2 padding,
    float gap,
    FlexLayout::CrossAlign crossAlign)
{
    auto layout = std::make_shared<FlexLayout>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ 0.0f, 0.0f },
        direction,
        padding,
        gap,
        crossAlign
    );

    contentLayouts.push_back(layout);
    SyncContentLayouts();
    return layout;
}

Rectangle Panel::GetContentRect() const
{
    return Rectangle{
        m_Bounds.x,
        m_Bounds.y + kPanelTitleBarHeight,
        m_Bounds.width,
        std::max(0.0f, m_Bounds.height - kPanelTitleBarHeight)
    };
}

void Panel::SyncContentLayouts()
{
    const Rectangle contentRect = GetContentRect();
    // All content layouts share one logical content area below the title bar.
    for (auto& layout : contentLayouts) {
        layout->m_Bounds = contentRect;
    }
}


void Panel::OnWindowResize(int screenWidth, int screenHeight) {
    Widget::OnWindowResize(screenWidth, screenHeight);
    for (auto& child : children) {
        child->OnWindowResize(screenWidth, screenHeight); 
    }
    SyncContentLayouts();
}


void Panel::AddChild(std::shared_ptr<Widget> widget) {
    children.push_back(widget);
}

void Panel::Update() {
    if (!isVisible) return;

    Vector2 mouse = GetMousePosition();

    
    if (isDragging) {
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            isDragging = false;
        }
        else {
           
            float newX = mouse.x + dragOffset.x;
            float newY = mouse.y + dragOffset.y;

          
            float deltaX = newX - m_Bounds.x;
            float deltaY = newY - m_Bounds.y;

            
            m_Bounds.x = newX;
            m_Bounds.y = newY;

            // Move absolute-positioned children by the same delta to preserve visual structure.
            for (auto& child : children) {
                child->m_Bounds.x += deltaX;
                child->m_Bounds.y += deltaY;
            }

            SyncContentLayouts();

            Widget::DesiredCursor = MOUSE_CURSOR_RESIZE_ALL;
        }
    }
    
    // Clicking, Hovering Logic
    else {
        
        Rectangle titleBar = { m_Bounds.x, m_Bounds.y, m_Bounds.width, 40 };

        if (CheckCollisionPointRec(mouse, titleBar)) {
           
            Widget::DesiredCursor = MOUSE_CURSOR_RESIZE_ALL;
            isHovered = true;

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                isDragging = true;
                // Store grab offset so panel does not snap its top-left to cursor.
                dragOffset = { m_Bounds.x - mouse.x, m_Bounds.y - mouse.y };
            }
        }
        else if (CheckCollisionPointRec(mouse, m_Bounds)) {
           
            isHovered = true;
           
        }
        else {
            isHovered = false;
        }
    }

    SyncContentLayouts();

    // Reverse traversal keeps top-most children/layouts first for interaction consistency.
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        (*it)->Update();
    }

    for (auto it = contentLayouts.rbegin(); it != contentLayouts.rend(); ++it) {
        (*it)->Update();
    }
}

void Panel::Draw(TextRenderer* renderer) {
    if (!isVisible) return;

    SyncContentLayouts();

    constexpr float kPanelRoundness = 0.12f;
    constexpr int kPanelRoundSegments = 12;

    // Convert a desired pixel corner radius into raylib's relative roundness for a rectangle.
    const auto roundnessForRect = [](const Rectangle& rect, float radiusPx) {
        const float minDim = std::max(1.0f, std::min(rect.width, rect.height));
        const float clampedRadius = std::clamp(radiusPx, 0.0f, minDim * 0.5f);
        return std::clamp((clampedRadius * 2.0f) / minDim, 0.0f, 1.0f);
    };

    // Keep panel corners in a stable visual range across both small and very large panels.
    const float panelMinDim = std::min(m_Bounds.width, m_Bounds.height);
    const float desiredCornerRadiusPx = std::clamp((kPanelRoundness * panelMinDim) * 0.5f, 10.0f, 18.0f);
    const float panelRoundness = roundnessForRect(m_Bounds, desiredCornerRadiusPx);

    // Background
    DrawRectangleRounded(m_Bounds, panelRoundness, kPanelRoundSegments, NookCol::UI_PANEL);

    // Use the same pixel corner radius as the panel so top corners align perfectly.
    const Rectangle titleBarRect{ m_Bounds.x, m_Bounds.y, m_Bounds.width, kPanelTitleBarHeight };
    const float titleRoundness = roundnessForRect(titleBarRect, desiredCornerRadiusPx);
    DrawRectangleRounded(titleBarRect, titleRoundness, kPanelRoundSegments, NookCol::UI_SHELL);
    const float titleFillStartY = titleBarRect.y + std::min(desiredCornerRadiusPx, kPanelTitleBarHeight * 0.5f);
    DrawRectangleRec(
        { titleBarRect.x, titleFillStartY, titleBarRect.width, titleBarRect.y + kPanelTitleBarHeight - titleFillStartY },
        NookCol::UI_SHELL
    );

    // Border
    DrawRectangleRoundedLinesEx(m_Bounds, panelRoundness, kPanelRoundSegments, 2.0f, NookCol::UI_BORDER);

    // Title Text
    if (renderer) {
        renderer->DrawSimpleText(title, { m_Bounds.x + 15, m_Bounds.y + 10 }, 22.0f, NookCol::UI_TEXT);
    }

    // Draw Children
    for (auto& child : children) {
        child->Draw(renderer);
    }

    for (auto& layout : contentLayouts) {
        layout->Draw(renderer);
    }
}