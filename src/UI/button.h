
// Button widget implementation.
// Handles user interaction with visual feedback and click callbacks.


#pragma once
#include "widget.h" 
#include <string>
#include <functional>

class TextRenderer;
class IconRenderer;

class Button : public Widget {
public:
    enum class ContentType {
        Text,
        Icon
    };

    Button(Anchor anchor, Vector2 offset, Vector2 size, std::string t, std::function<void()> callback);
    void Update() override;
    void Draw(TextRenderer* renderer) override;
    void DrawWithIcons(TextRenderer* renderer, IconRenderer* iconRenderer) override;

    void SetText(const std::string& t) { m_Text = t; m_ContentType = ContentType::Text; }
    void SetIcon(int iconType) { m_IconType = iconType; m_ContentType = ContentType::Icon; }
    void SetOnClick(std::function<void()> callback) { m_OnClick = callback; }

private:
    std::string m_Text{};
    int m_IconType = -1;
    ContentType m_ContentType = ContentType::Text;
    std::function<void()> m_OnClick;
    bool m_IsHovered{ false };
};