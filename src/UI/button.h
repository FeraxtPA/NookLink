#pragma once
#include "widget.h" 
#include <string>
#include <functional>

class Button : public Widget {
public:
    Button(Anchor anchor, Vector2 offset, Vector2 size, std::string t, std::function<void()> callback);
    void Update() override;
    void Draw(TextRenderer* renderer) override;
    void SetText(const std::string& t) { text = t; }
    void SetOnClick(std::function<void()> callback) { onClick = callback; }

private:
    std::string text;
    std::function<void()> onClick;
    bool isHovered = false;
};