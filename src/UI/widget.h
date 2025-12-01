#pragma once
#include <raylib.h>
#include "textRenderer.h"

class Widget {
public:
    Widget(Rectangle r) : bounds(r) {}
    virtual ~Widget() = default;

    virtual void Update() = 0;
    virtual void Draw() = 0;

protected:
    TextRenderer textRenderer;
    Rectangle bounds;
    bool isVisible = true;

   
};