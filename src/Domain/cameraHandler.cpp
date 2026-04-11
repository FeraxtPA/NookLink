
// Implementation of the CameraHandler class.
// Manages camera transformations for 2D graph visualization.


#include "cameraHandler.h"

#include "raymath.h"

#include <cmath>

CameraHandler::CameraHandler(Vector2 screenDimensions, Vector2 canvasSize)
{
    m_Camera.target = { (float)canvasSize.x / 2, (float)canvasSize.y / 2 };
    m_Camera.offset = { (float)screenDimensions.x / 2, (float)screenDimensions.y / 2 };
    m_Camera.rotation = 0.0f;
    m_Camera.zoom = 0.5f;

    m_MinZoom = 0.01f;
    m_MaxZoom = 5.0f;
    m_ZoomSpeed = 0.05f;
    m_PanButton = MOUSE_BUTTON_MIDDLE;
}

void CameraHandler::update()
{
    bool usedManualNavigation = false;

    // Pan with mouse drag
    if (IsMouseButtonDown(m_PanButton)) {
        Vector2 mouseDelta = GetMouseDelta();
        m_Camera.target.x -= mouseDelta.x / m_Camera.zoom;
        m_Camera.target.y -= mouseDelta.y / m_Camera.zoom;
        usedManualNavigation = true;
    }

    // Zoom with mouse wheel
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        m_Camera.zoom += wheel * m_ZoomSpeed;
        if (m_Camera.zoom < m_MinZoom) m_Camera.zoom = m_MinZoom;
        if (m_Camera.zoom > m_MaxZoom) m_Camera.zoom = m_MaxZoom;
        usedManualNavigation = true;
    }

    if (usedManualNavigation) {
        ClearFocusTarget();
    }
}

void CameraHandler::updateAutoFocus(float dt)
{
    if (!m_HasFocusTarget) {
        return;
    }

    const float targetT = 1.0f - std::exp(-m_FocusTargetLerpSpeed * dt);
    const float zoomT = 1.0f - std::exp(-m_FocusZoomLerpSpeed * dt);

    m_Camera.target = Vector2Lerp(m_Camera.target, m_FocusTarget, targetT);
    m_Camera.zoom = Lerp(m_Camera.zoom, m_FocusZoom, zoomT);

    const float dx = m_Camera.target.x - m_FocusTarget.x;
    const float dy = m_Camera.target.y - m_FocusTarget.y;
    const float distSq = dx * dx + dy * dy;
    const float zoomDelta = std::fabs(m_Camera.zoom - m_FocusZoom);
    if (distSq < 1.0f && zoomDelta < 0.01f) {
        m_Camera.target = m_FocusTarget;
        m_Camera.zoom = m_FocusZoom;
        m_HasFocusTarget = false;
    }
}

void CameraHandler::SetFocusTarget(Vector2 target, float zoom, bool immediate)
{
    m_FocusTarget = target;
    m_FocusZoom = zoom;
    if (m_FocusZoom < m_MinZoom) m_FocusZoom = m_MinZoom;
    if (m_FocusZoom > m_MaxZoom) m_FocusZoom = m_MaxZoom;
    m_HasFocusTarget = true;

    if (immediate) {
        m_Camera.target = m_FocusTarget;
        m_Camera.zoom = m_FocusZoom;
        m_HasFocusTarget = false;
    }
}

void CameraHandler::ClearFocusTarget()
{
    m_HasFocusTarget = false;
}

void CameraHandler::updateScreenSize(Vector2 newScreenSize)
{
    m_Camera.offset = { newScreenSize.x / 2.0f, newScreenSize.y / 2.0f };
}
