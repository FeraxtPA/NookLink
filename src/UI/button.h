#pragma once
#include "Widget.h"
#include <string>
#include <functional>

class Button : public Widget {
public:
    Button(Rectangle r, std::string t, std::function<void()> callback);
        

    void Update() override;

    void Draw() override;

private:
    std::string text;
    std::function<void()> onClick; // Akce
    bool isHovered = false;

};