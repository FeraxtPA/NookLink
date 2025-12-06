#include "button.h"

Button::Button(Rectangle r, std::string t, std::function<void()> callback)
	: Widget(r), text(t), onClick(callback) 
{
}

void Button::Update()
{
    if (!isVisible) return;
    Vector2 mouse = GetMousePosition();
    
    isHovered = CheckCollisionPointRec(mouse, bounds);

    if (isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && onClick) {
        onClick();
    }
}

void Button::Draw()
{
    if (!isVisible) return;
    DrawRectangleRec(bounds, isHovered ? DARKGRAY : LIGHTGRAY);
    DrawRectangleLinesEx(bounds, 2, BLACK);
    int textW = MeasureText(text.c_str(), 20);
    
    textRenderer.DrawText(
		text, 
		{ bounds.x + bounds.width / 2.0f, bounds.y + bounds.height / 2.0f }, 
		20, 
		BLACK, 
		true
	);
}

