#include "panel.h"
#include "../textRenderer.h"
#include "../colors.h"

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
    for (auto& layout : contentLayouts) {
        layout->m_Bounds = contentRect;
    }
}


void Panel::OnWindowResize(int screenWidth, int screenHeight) {
    Widget::OnWindowResize(screenWidth, screenHeight); // Spo��t� m_Bounds panelu
    for (auto& child : children) {
        child->OnWindowResize(screenWidth, screenHeight); // Aktualizuje v�echny d�ti
    }
    SyncContentLayouts();
}


void Panel::AddChild(std::shared_ptr<Widget> widget) {
    children.push_back(widget);
}

void Panel::Update() {
    if (!isVisible) return;

    Vector2 mouse = GetMousePosition();

    // Dragging Logic
    if (isDragging) {
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            isDragging = false;
        }
        else {
            // Calculate new position based on mouse + offset
            float newX = mouse.x + dragOffset.x;
            float newY = mouse.y + dragOffset.y;

            // Calculate how much we moved this frame
            float deltaX = newX - m_Bounds.x;
            float deltaY = newY - m_Bounds.y;

            // Apply move to Panel
            m_Bounds.x = newX;
            m_Bounds.y = newY;

            // Apply move to ALL Children
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
        // Define the Title Bar area (Top 40 pixels)
        Rectangle titleBar = { m_Bounds.x, m_Bounds.y, m_Bounds.width, 40 };

        if (CheckCollisionPointRec(mouse, titleBar)) {
            // If hovering title bar, show move cursor
            Widget::DesiredCursor = MOUSE_CURSOR_RESIZE_ALL;
            isHovered = true;

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                isDragging = true;
                // Remember where we grabbed the panel relative to its corner
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

    // Update Children
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

    // Background
    DrawRectangleRounded(m_Bounds, 0.12f, 12, NookCol::UI_PANEL);

    // Draw Title Bar 
    DrawRectangleRounded({ m_Bounds.x, m_Bounds.y, m_Bounds.width, kPanelTitleBarHeight }, 0.12f, 12, NookCol::UI_SHELL);

    // Border
    DrawRectangleRoundedLinesEx(m_Bounds, 0.12f, 12, 2.0f, NookCol::UI_BORDER);

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