#pragma once
#include "Widget.h"
#include <vector>
#include <memory>
#include <string>

class Panel : public Widget {
public:
    Panel(Rectangle r, std::string t);

   
    void AddChild(std::shared_ptr<Widget> widget);

    void Update() override;
    void Draw(TextRenderer* renderer) override;

    void ClearChildren(){ 		children.clear();
	}

private:
    std::vector<std::shared_ptr<Widget>> children;
    std::string title;

    bool isDragging = false;
    Vector2 dragOffset = { 0, 0 };
};