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

    isHovered = CheckCollisionPointRec(GetMousePosition(), bounds);

   

    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        (*it)->Update();
    }
}

void Panel::Draw(TextRenderer* renderer) {
    if (!isVisible) return;

   
    DrawRectangleRec(bounds, Fade(LIGHTGRAY, 0.9f));
    DrawRectangleLinesEx(bounds, 3, DARKGRAY);

    
    if (renderer) {
        renderer->DrawSimpleText(title, { bounds.x + 15, bounds.y + 10 }, 22.0f, DARKGRAY);
    }

    
    for (auto& child : children) {
        child->Draw(renderer);
    }
}