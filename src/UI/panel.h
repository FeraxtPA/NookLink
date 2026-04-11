
// Panel widget for grouping and organizing UI elements.
// Provides title bar, content layout, and child widget management.


#pragma once
#include "widget.h"
#include "flexLayout.h"
#include <vector>
#include <memory>
#include <string>

class Panel : public Widget {
public:
    Panel(Anchor anchor, Vector2 offset, Vector2 size, std::string t);
    void SetTitle(const std::string& t) { m_Title = t; }

    std::shared_ptr<FlexLayout> CreateContentLayout(
        FlexLayout::Direction direction,
        Vector2 padding,
        float gap,
        FlexLayout::CrossAlign crossAlign = FlexLayout::CrossAlign::Start
    );

    void AddChild(std::shared_ptr<Widget> widget);
    void BeginFrameInputRecursive() override;
    MouseCursor ResolveRequestedCursorRecursive() const override;
    void Update() override;
    void Draw(TextRenderer* renderer) override;
    void ClearChildren() {
        m_Children.clear();
    }
    void OnWindowResize(int screenWidth, int screenHeight) override;
private:
    std::vector<std::shared_ptr<Widget>> m_Children;
    std::vector<std::shared_ptr<FlexLayout>> m_ContentLayouts;
    std::string m_Title;

    bool m_IsDragging = false;
    Vector2 m_DragOffset = { 0, 0 };

    Rectangle GetContentRect() const;
    void SyncContentLayouts();
};