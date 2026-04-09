
// Linear slider widget for numeric value selection.
// Provides draggable handle with min/max bounds and change callbacks.


#pragma once
#include "widget.h"
#include <string>
#include <functional>

class Slider : public Widget {
public:
    float value;
    float minVal;
    float maxVal;
    std::function<void(float)> onChange;

    // Constructor takes min, max, and initial value
    Slider(Anchor anchor, Vector2 offset, Vector2 size, float minV, float maxV, float initialVal, std::function<void(float)> onChangeCallback = nullptr);

    void Update() override;
    void Draw(TextRenderer* renderer) override;

private:
    bool isDragging = false;
};