#pragma once
#include <raylib.h>

// Forward declaration
class TextRenderer;

class Widget {
public:
    Rectangle bounds;
    bool isVisible = true;
    bool isHovered = false;


    static MouseCursor DesiredCursor;

    Widget(Rectangle r) : bounds(r) {}
    virtual ~Widget() = default;

    virtual void Update() = 0;
    
    virtual void Draw(TextRenderer* renderer) = 0;
};