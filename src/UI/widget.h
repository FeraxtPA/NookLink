
// Base widget class for all UI components.
// Defines positioning, anchoring, visibility, and event handling for UI elements.


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
    const Rectangle& GetBounds() const { return m_Bounds; }
    void SetBounds(const Rectangle& bounds) { m_Bounds = bounds; }

    bool IsVisible() const { return m_IsVisible; }
    void SetVisible(bool isVisible) { m_IsVisible = isVisible; }

    bool IsHovered() const { return m_IsHovered; }
    void SetHovered(bool isHovered) { m_IsHovered = isHovered; }


    
    

    static void ResetInputConsumption() {
        s_LeftClickConsumed = false;
    }

    static bool IsLeftClickConsumed() {
        return s_LeftClickConsumed;
    }

    static void ConsumeLeftClick() {
        s_LeftClickConsumed = true;
    }

    bool HandleHoverCursor(const Rectangle& hitArea, MouseCursor cursor = MOUSE_CURSOR_POINTING_HAND)
    {
        const bool hovered = IsVisible() && CheckCollisionPointRec(GetMousePosition(), hitArea);
        SetHovered(hovered);
        if (hovered) {
            RequestCursor(cursor);
        }
        return hovered;
    }

    bool ConsumeLeftClickOnHover(const Rectangle& hitArea, MouseCursor cursor = MOUSE_CURSOR_POINTING_HAND)
    {
        if (!HandleHoverCursor(hitArea, cursor)) {
            return false;
        }

        if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsLeftClickConsumed()) {
            return false;
        }

        ConsumeLeftClick();
        return true;
    }
    

    void BeginFrameInput()
    {
		m_RequestedCursor = MOUSE_CURSOR_DEFAULT;
    }

    virtual void BeginFrameInputRecursive()
    {
        BeginFrameInput();
    }

    void RequestCursor(MouseCursor cursor)
    {
        m_RequestedCursor = cursor;
	}
    MouseCursor GetRequestedCursor() const
    {
        return m_RequestedCursor;
	}

    virtual MouseCursor ResolveRequestedCursorRecursive() const
    {
        if (!IsVisible()) {
            return MOUSE_CURSOR_DEFAULT;
        }
        return m_RequestedCursor;
    }

    Widget(Anchor anchor, Vector2 offset, Vector2 size)
        : m_Anchor(anchor), m_Offset(offset), m_Size(size) {}

    void SetSize(Vector2 size) { m_Size = size; }
    void SetOffset(Vector2 offset) { m_Offset = offset; }
    Vector2 GetSize() const { return m_Size; }
    Vector2 GetOffset() const { return m_Offset; }


    virtual void OnWindowResize(int screenWidth, int screenHeight) {
       
        float anchorX = 0.0f;
        float anchorY = 0.0f;

        switch (m_Anchor) {
        case Anchor::TopLeft:      anchorX = 0.0f; anchorY = 0.0f; break;
        case Anchor::TopCenter:    anchorX = screenWidth / 2.0f; anchorY = 0.0f; break;
        case Anchor::TopRight:     anchorX = static_cast<float>(screenWidth); anchorY = 0.0f; break;
        case Anchor::CenterLeft:   anchorX = 0.0f; anchorY = screenHeight / 2.0f; break;
        case Anchor::Center:       anchorX = screenWidth / 2.0f; anchorY = screenHeight / 2.0f; break;
        case Anchor::CenterRight:  anchorX = static_cast<float>(screenWidth); anchorY = screenHeight / 2.0f; break;
        case Anchor::BottomLeft:   anchorX = 0.0f; anchorY = static_cast<float>(screenHeight); break;
        case Anchor::BottomCenter: anchorX = screenWidth / 2.0f; anchorY = static_cast<float>(screenHeight); break;
        case Anchor::BottomRight:  anchorX = static_cast<float>(screenWidth); anchorY = static_cast<float>(screenHeight); break;
        }

        m_Bounds.x = anchorX + m_Offset.x - (m_Size.x / 2.0f);
        m_Bounds.y = anchorY + m_Offset.y - (m_Size.y / 2.0f);
        m_Bounds.width = m_Size.x;
        m_Bounds.height = m_Size.y;
    }

    virtual ~Widget() = default;

    virtual void Update() = 0;
    
    virtual void Draw(TextRenderer* renderer) = 0;

private:
	MouseCursor m_RequestedCursor = MOUSE_CURSOR_DEFAULT;
    static bool s_LeftClickConsumed;

protected:
    Rectangle m_Bounds{};
    bool m_IsVisible = true;
    bool m_IsHovered = false;
};