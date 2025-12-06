#pragma once
#include <raylib.h>

// Forward declaration
class TextRenderer;

class Widget {
public:
    Rectangle bounds;
    bool isVisible = true;
    bool isHovered = false;

    Widget(Rectangle r) : bounds(r) {}
    virtual ~Widget() = default;

    virtual void Update() = 0;
    // CHANGE: Draw now takes a pointer to the renderer
    virtual void Draw(TextRenderer* renderer) = 0;
};