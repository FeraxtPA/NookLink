#pragma once
#include "Widget.h"
#include <vector>
#include <memory>
#include <string>

class Panel : public Widget {
public:
    Panel(Rectangle r, std::string t);

    // Add a widget to the panel
    void AddChild(std::shared_ptr<Widget> widget);

    void Update() override;
    void Draw(TextRenderer* renderer) override;

private:
    std::vector<std::shared_ptr<Widget>> children;
    std::string title;
};