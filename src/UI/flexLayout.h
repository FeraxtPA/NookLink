#pragma once

#include "widget.h"
#include <memory>
#include <vector>

class FlexLayout : public Widget {
public:
    enum class Direction {
        Horizontal,
        Vertical
    };

    enum class CrossAlign {
        Start,
        Center,
        End,
        Stretch
    };

    struct Item {
        std::shared_ptr<Widget> widget;
        Vector2 size;
        float grow = 0.0f;
    };

    FlexLayout(Anchor anchor, Vector2 offset, Vector2 size, Direction direction, Vector2 padding = { 0.0f, 0.0f }, float gap = 0.0f, CrossAlign crossAlign = CrossAlign::Center);

    void AddChild(std::shared_ptr<Widget> widget, Vector2 size, float grow = 0.0f);
    void AddSpacer(float grow = 1.0f);
    void ClearChildren();

    void SetPadding(Vector2 padding) { m_Padding = padding; }
    void SetGap(float gap) { m_Gap = gap; }

    void Update() override;
    void Draw(TextRenderer* renderer) override;
    void OnWindowResize(int screenWidth, int screenHeight) override;

private:
    std::vector<Item> m_Items;
    Direction m_Direction;
    CrossAlign m_CrossAlign;
    Vector2 m_Padding;
    float m_Gap;

    void LayoutChildren();
};