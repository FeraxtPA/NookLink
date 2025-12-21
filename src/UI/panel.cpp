#include "panel.h"
#include "../textRenderer.h"

Panel::Panel(Rectangle r, std::string t)
    : Widget(r), title(t)
{}

void Panel::AddChild(std::shared_ptr<Widget> widget) {
    children.push_back(widget);
}

void Panel::Update() {
    if (!isVisible) return;

    Vector2 mouse = GetMousePosition();

    // 1. Handle Active Dragging
    if (isDragging) {
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            isDragging = false;
        }
        else {
            // Calculate new position based on mouse + offset
            float newX = mouse.x + dragOffset.x;
            float newY = mouse.y + dragOffset.y;

            // Calculate how much we moved this frame
            float deltaX = newX - bounds.x;
            float deltaY = newY - bounds.y;

            // Apply move to Panel
            bounds.x = newX;
            bounds.y = newY;

            // Apply move to ALL Children
            for (auto& child : children) {
                child->bounds.x += deltaX;
                child->bounds.y += deltaY;
            }

            Widget::DesiredCursor = MOUSE_CURSOR_RESIZE_ALL;
        }
    }
    // 2. Handle Hover & Click interactions
    else {
        // Define the Title Bar area (Top 40 pixels)
        Rectangle titleBar = { bounds.x, bounds.y, bounds.width, 40 };

        if (CheckCollisionPointRec(mouse, titleBar)) {
            // If hovering title bar, show move cursor
            Widget::DesiredCursor = MOUSE_CURSOR_RESIZE_ALL;
            isHovered = true;

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                isDragging = true;
                // Remember where we grabbed the panel relative to its corner
                dragOffset = { bounds.x - mouse.x, bounds.y - mouse.y };
            }
        }
        else if (CheckCollisionPointRec(mouse, bounds)) {
            // Normal hover over the body
            isHovered = true;
            // Only set cursor if not set by a child later
            // Widget::DesiredCursor = MOUSE_CURSOR_DEFAULT; 
        }
        else {
            isHovered = false;
        }
    }

    // 3. Update Children
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        (*it)->Update();
    }
}

void Panel::Draw(TextRenderer* renderer) {
    if (!isVisible) return;

    // Background
    DrawRectangleRec(bounds, Fade(LIGHTGRAY, 0.95f));

    // Draw Title Bar Background (Visual cue for dragging)
    DrawRectangle((int)bounds.x, (int)bounds.y, (int)bounds.width, 40, Fade(GRAY, 0.4f));

    // Border
    DrawRectangleLinesEx(bounds, 3, DARKGRAY);

    // Title Text
    if (renderer) {
        renderer->DrawSimpleText(title, { bounds.x + 15, bounds.y + 10 }, 22.0f, DARKGRAY);
    }

    // Draw Children
    for (auto& child : children) {
        child->Draw(renderer);
    }
}