
// Implementation of the Panel widget class.
// Manages panel rendering with title bar and content layout.


#include "panel.h"
#include "../textRenderer.h"
#include "../colors.h"
#include "../constants.h"

#include <algorithm>

Panel::Panel(Anchor anchor, Vector2 offset, Vector2 size, std::string t)
    : Widget(anchor, offset, size), m_Title(t)
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

    m_ContentLayouts.push_back(layout);
    SyncContentLayouts();
    return layout;
}

Rectangle Panel::GetContentRect() const
{
    return Rectangle{
        m_Bounds.x,
        m_Bounds.y + NookConst::UI::kPanelTitleBarHeight,
        m_Bounds.width,
        std::max(0.0f, m_Bounds.height - NookConst::UI::kPanelTitleBarHeight)
    };
}

void Panel::SyncContentLayouts()
{
    const Rectangle contentRect = GetContentRect();
    // All content layouts share one logical content area below the title bar.
    for (auto& layout : m_ContentLayouts) {
        layout->SetBounds(contentRect);
    }
}


void Panel::OnWindowResize(int screenWidth, int screenHeight) {
    Widget::OnWindowResize(screenWidth, screenHeight);
    for (auto& child : m_Children) {
        child->OnWindowResize(screenWidth, screenHeight); 
    }
    SyncContentLayouts();
}


void Panel::AddChild(std::shared_ptr<Widget> widget) {
    m_Children.push_back(widget);
}

void Panel::BeginFrameInputRecursive()
{
    BeginFrameInput();

    for (auto& child : m_Children) {
        if (child) {
            child->BeginFrameInputRecursive();
        }
    }

    for (auto& layout : m_ContentLayouts) {
        if (layout) {
            layout->BeginFrameInputRecursive();
        }
    }
}

MouseCursor Panel::ResolveRequestedCursorRecursive() const
{
    if (!m_IsVisible) {
        return MOUSE_CURSOR_DEFAULT;
    }

    for (auto it = m_ContentLayouts.rbegin(); it != m_ContentLayouts.rend(); ++it) {
        if (!(*it)) continue;
        const MouseCursor requested = (*it)->ResolveRequestedCursorRecursive();
        if (requested != MOUSE_CURSOR_DEFAULT) {
            return requested;
        }
    }

    for (auto it = m_Children.rbegin(); it != m_Children.rend(); ++it) {
        if (!(*it)) continue;
        const MouseCursor requested = (*it)->ResolveRequestedCursorRecursive();
        if (requested != MOUSE_CURSOR_DEFAULT) {
            return requested;
        }
    }

    return GetRequestedCursor();
}

void Panel::Update() {
    if (!m_IsVisible) return;

    Vector2 mouse = GetMousePosition();

    
    if (m_IsDragging) {
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            m_IsDragging = false;
        }
        else {
           
            float newX = mouse.x + m_DragOffset.x;
            float newY = mouse.y + m_DragOffset.y;

          
            float deltaX = newX - m_Bounds.x;
            float deltaY = newY - m_Bounds.y;

            
            m_Bounds.x = newX;
            m_Bounds.y = newY;

            // Move absolute-positioned children by the same delta to preserve visual structure.
            for (auto& child : m_Children) {
                Rectangle childBounds = child->GetBounds();
                childBounds.x += deltaX;
                childBounds.y += deltaY;
                child->SetBounds(childBounds);
            }

            SyncContentLayouts();

			RequestCursor(MOUSE_CURSOR_RESIZE_ALL);
        }
    }
    
    // Clicking, Hovering Logic
    else {
        
        Rectangle titleBar = { m_Bounds.x, m_Bounds.y, m_Bounds.width, NookConst::UI::kPanelTitleBarHeight };

        if (CheckCollisionPointRec(mouse, titleBar)) {
           
			RequestCursor(MOUSE_CURSOR_RESIZE_ALL);
            m_IsHovered = true;

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                m_IsDragging = true;
                // Store grab offset so panel does not snap its top-left to cursor.
                m_DragOffset = { m_Bounds.x - mouse.x, m_Bounds.y - mouse.y };
            }
        }
        else if (CheckCollisionPointRec(mouse, m_Bounds)) {
           
            m_IsHovered = true;
           
        }
        else {
            m_IsHovered = false;
        }
    }

    SyncContentLayouts();

    // Reverse traversal keeps top-most children/layouts first for interaction consistency.
    for (auto it = m_Children.rbegin(); it != m_Children.rend(); ++it) {
        (*it)->Update();
    }

	// Content layouts are on the same visual plane as children, so update them in the same back-to-front order.
    for (auto it = m_ContentLayouts.rbegin(); it != m_ContentLayouts.rend(); ++it) {
        (*it)->Update();
    }
}

void Panel::Draw(TextRenderer* renderer) {
    if (!m_IsVisible) return;

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
    const Rectangle titleBarRect{ m_Bounds.x, m_Bounds.y, m_Bounds.width, NookConst::UI::kPanelTitleBarHeight };
    const float titleRoundness = roundnessForRect(titleBarRect, desiredCornerRadiusPx);
    DrawRectangleRounded(titleBarRect, titleRoundness, kPanelRoundSegments, NookCol::UI_SHELL);
    const float titleFillStartY = titleBarRect.y + std::min(desiredCornerRadiusPx, NookConst::UI::kPanelTitleBarHeight * 0.5f);
    DrawRectangleRec(
        { titleBarRect.x, titleFillStartY, titleBarRect.width, titleBarRect.y + NookConst::UI::kPanelTitleBarHeight - titleFillStartY },
        NookCol::UI_SHELL
    );

    // Border
    DrawRectangleRoundedLinesEx(m_Bounds, panelRoundness, kPanelRoundSegments, 2.0f, NookCol::UI_BORDER);

    // Title Text
    if (renderer) {
        renderer->DrawSimpleText(m_Title, { m_Bounds.x + 15, m_Bounds.y + 10 }, 22.0f, NookCol::UI_TEXT);
    }

    // Draw Children
	// Probably useless since content layouts are usually used instead of direct children, but support both just in case.
    for (auto& child : m_Children) {
        child->Draw(renderer);
    }
    
    
    for (auto& layout : m_ContentLayouts) {
        layout->Draw(renderer);
    }
    
}