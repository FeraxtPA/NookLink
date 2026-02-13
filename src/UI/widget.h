#pragma once
#include <raylib.h>


enum class Anchor
{
    TopLeft,
    TopCenter,
    TopRight,
    CenterLeft,
    Center,
    CenterRight,
    BottomLeft,
    BottomCenter,
    BottomRight
};

// Forward declaration
class TextRenderer;

class Widget {
protected:
    
    Anchor m_Anchor;
    Vector2 m_Offset;
    Vector2 m_Size;

public:
    Rectangle m_Bounds;
    bool isVisible = true;
    bool isHovered = false;


    static MouseCursor DesiredCursor;

    Widget(Anchor anchor, Vector2 offset, Vector2 size)
        : m_Anchor(anchor), m_Offset(offset), m_Size(size) {}


    virtual void OnWindowResize(int screenWidth, int screenHeight) {
        // Vypoèítáme základní bod (x, y) podle kotvy
        float anchorX = 0;
        float anchorY = 0;

        switch (m_Anchor) {
        case Anchor::TopLeft:      anchorX = 0; anchorY = 0; break;
        case Anchor::TopCenter:    anchorX = screenWidth / 2.0f; anchorY = 0; break;
        case Anchor::TopRight:     anchorX = screenWidth; anchorY = 0; break;
        case Anchor::CenterLeft:   anchorX = 0; anchorY = screenHeight / 2.0f; break;
        case Anchor::Center:       anchorX = screenWidth / 2.0f; anchorY = screenHeight / 2.0f; break;
        case Anchor::CenterRight:  anchorX = screenWidth; anchorY = screenHeight / 2.0f; break;
        case Anchor::BottomLeft:   anchorX = 0; anchorY = screenHeight; break;
        case Anchor::BottomCenter: anchorX = screenWidth / 2.0f; anchorY = screenHeight; break;
        case Anchor::BottomRight:  anchorX = screenWidth; anchorY = screenHeight; break;
        }

        // Vypoèítáme finální pozici (kotva + odsazení - polovina velikosti prvku, aby byl vycentrovaný na kotvì)
        // (Pokud chceš, aby kotva urèovala roh prvku a ne støed, výpoèet se lehce upraví)
        m_Bounds.x = anchorX + m_Offset.x - (m_Size.x / 2.0f);
        m_Bounds.y = anchorY + m_Offset.y - (m_Size.y / 2.0f);
        m_Bounds.width = m_Size.x;
        m_Bounds.height = m_Size.y;
    }

    virtual ~Widget() = default;

    virtual void Update() = 0;
    
    virtual void Draw(TextRenderer* renderer) = 0;
};