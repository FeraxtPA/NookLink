#pragma once
#include "widget.h"
#include "flexLayout.h"
#include <vector>
#include <memory>
#include <string>

class Panel : public Widget {
public:
    Panel(Anchor anchor, Vector2 offset, Vector2 size, std::string t);

    std::shared_ptr<FlexLayout> CreateContentLayout(
        FlexLayout::Direction direction,
        Vector2 padding,
        float gap,
        FlexLayout::CrossAlign crossAlign = FlexLayout::CrossAlign::Start
    );

    void AddChild(std::shared_ptr<Widget> widget);
    void Update() override;
    void Draw(TextRenderer* renderer) override;
    void ClearChildren() {
        children.clear();
    }
    void OnWindowResize(int screenWidth, int screenHeight) override;
private:
    std::vector<std::shared_ptr<Widget>> children;
    std::vector<std::shared_ptr<FlexLayout>> contentLayouts;
    std::string title;

    bool isDragging = false;
    Vector2 dragOffset = { 0, 0 };

    Rectangle GetContentRect() const;
    void SyncContentLayouts();
};