#pragma once
#include <raylib.h>

class CameraHandler {
public:
    CameraHandler(int screenWidth, int screenHeight, Vector2 canvasSize);
   
    void update();
   

    void beginMode() const { BeginMode2D(m_Camera); }
    void endMode() const { EndMode2D(); }

    const Camera2D& getCamera() const { return m_Camera; }

    void setPanButton(MouseButton mouseButton) { m_PanButton = mouseButton; }
    void setZoomLimits(float minZ, float maxZ) { m_MinZoom = minZ; m_MaxZoom = maxZ; }
    void setZoomSpeed(float speed) { m_ZoomSpeed = speed; }

private:
    Camera2D m_Camera;
    MouseButton m_PanButton;
    float m_MinZoom;
    float m_MaxZoom;
    float m_ZoomSpeed;
};
