#pragma once
#include "Widget.h"
#include <string>

class Checkbox : public Widget {
public:
    bool checked = false;
    std::string label;

    Checkbox(Rectangle r, std::string l, bool initial = false);

    void Update() override;
    void Draw(TextRenderer* renderer) override;
};