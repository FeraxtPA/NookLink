
// Button widget implementation.
// Handles user interaction with visual feedback and click callbacks.


#pragma once
#include "widget.h" 
#include <string>
#include <functional>

class Button : public Widget {
public:
    Button(Anchor anchor, Vector2 offset, Vector2 size, std::string t, std::function<void()> callback);
    void Update() override;
    void Draw(TextRenderer* renderer) override;
    void SetText(const std::string& t) { m_Text = t; }
    void SetOnClick(std::function<void()> callback) { m_OnClick = callback; }

private:
    std::string m_Text{};
    std::function<void()> m_OnClick;
    bool m_IsHovered{ false };
};