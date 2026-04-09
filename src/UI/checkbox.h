
// Checkbox widget for boolean selection.
// Displays a checkbox with label and provides change callbacks.


#pragma once
#include "widget.h"
#include <string>
#include <functional>

class Checkbox : public Widget {
public:
    bool checked = false;
    std::string label;
    std::function<void(bool)> onChange;

    Checkbox(Anchor anchor, Vector2 offset, Vector2 size, std::string l, bool initial = false, std::function<void(bool)> onChangeCallback = nullptr);

    void Update() override;
    void Draw(TextRenderer* renderer) override;

private:
    double m_LastClickTime = 0.0;
};