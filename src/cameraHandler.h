
// Manages the 2D camera for graph visualization.
// Handles panning, zooming, and viewport management for the graph canvas.


#pragma once
#include <raylib.h>

class CameraHandler {
public:
    CameraHandler(Vector2 screenDimensions, Vector2 canvasSize);
   
    void update();
   

    void beginMode() const { BeginMode2D(m_Camera); }
    void endMode() const { EndMode2D(); }

    const Camera2D& getCamera() const { return m_Camera; }

    
    void updateScreenSize(Vector2 newScreenSize);
    

private:
    Camera2D m_Camera;
    MouseButton m_PanButton;
    float m_MinZoom;
    float m_MaxZoom;
    float m_ZoomSpeed;
};
