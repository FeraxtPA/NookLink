#include "panel.h"
#include "../textRenderer.h"

Panel::Panel(Anchor anchor, Vector2 offset, Vector2 size, std::string t)
    : Widget(anchor, offset, size), title(t)
{
    OnWindowResize(GetScreenWidth(), GetScreenHeight());
}


void Panel::OnWindowResize(int screenWidth, int screenHeight) {
    Widget::OnWindowResize(screenWidth, screenHeight); // Spoèítá m_Bounds panelu
    for (auto& child : children) {
        child->OnWindowResize(screenWidth, screenHeight); // Aktualizuje všechny dìti
    }
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

    // Update Children
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        (*it)->Update();
    }
}

void Panel::Draw(TextRenderer* renderer) {
    if (!isVisible) return;

    // Background
    DrawRectangleRec(m_Bounds, Fade(LIGHTGRAY, 0.95f));

    // Draw Title Bar 
    DrawRectangle((int)m_Bounds.x, (int)m_Bounds.y, (int)m_Bounds.width, 40, Fade(GRAY, 0.4f));

    // Border
    DrawRectangleLinesEx(m_Bounds, 3, DARKGRAY);

    // Title Text
    if (renderer) {
        renderer->DrawSimpleText(title, { m_Bounds.x + 15, m_Bounds.y + 10 }, 22.0f, DARKGRAY);
    }

    // Draw Children
    for (auto& child : children) {
        child->Draw(renderer);
    }
}