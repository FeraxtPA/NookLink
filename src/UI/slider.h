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

    // Konstruktor bere min, max a výchozí hodnotu
    Slider(Anchor anchor, Vector2 offset, Vector2 size, float minV, float maxV, float initialVal, std::function<void(float)> onChangeCallback = nullptr);

    void Update() override;
    void Draw(TextRenderer* renderer) override;

private:
    bool isDragging = false;
};