#include "cameraHandler.h"

CameraHandler::CameraHandler(int screenWidth, int screenHeight, Vector2 canvasSize) 
{
    m_Camera.target = { (float)canvasSize.x / 2, (float)canvasSize.y / 2 };
    m_Camera.offset = { (float)screenWidth / 2, (float)screenHeight / 2 };
    m_Camera.rotation = 0.0f;
    m_Camera.zoom = 0.5f;

    m_MinZoom = 0.01f;
    m_MaxZoom = 5.0f;
    m_ZoomSpeed = 0.05f;
    m_PanButton = MOUSE_BUTTON_MIDDLE;
}

void CameraHandler::update()
{
    // Pan with mouse drag
    if (IsMouseButtonDown(m_PanButton)) {
        Vector2 mouseDelta = GetMouseDelta();
        m_Camera.target.x -= mouseDelta.x / m_Camera.zoom;
        m_Camera.target.y -= mouseDelta.y / m_Camera.zoom;
    }

    // Zoom with mouse wheel
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        m_Camera.zoom += wheel * m_ZoomSpeed;
        if (m_Camera.zoom < m_MinZoom) m_Camera.zoom = m_MinZoom;
        if (m_Camera.zoom > m_MaxZoom) m_Camera.zoom = m_MaxZoom;
    }
}
