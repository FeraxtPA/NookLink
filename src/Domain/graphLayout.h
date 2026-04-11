
// Manages graph layout algorithms for both physics-based and grid-based layouts.
// Provides force-directed physics simulation and lerp-based smooth transitions.


#pragma once
#include "nodeRenderer.h"
#include <raymath.h>
#include <unordered_map>
#include <vector>

class GraphLayout
{
public:
    GraphLayout() = default;

    void setLayoutDensityScale(float scale);
    float getLayoutDensityScale() const { return m_LayoutDensityScale; }

    void wakeUp() { m_Temperature = 1.0f; m_CoolingHoldFrames = 18; }

    void calculateGridLayout(
        std::vector<Node>& nodes,
        Vector2 centerPos,
        const std::unordered_map<int, std::vector<int>>& bookToGenreMap
    );

    bool updateLerp(std::vector<Node>& nodes, float dt);

    // Main physics loop: applies attraction, repulsion, and gravity forces
    bool updatePhysics(
        std::vector<Node>& nodes,
        const std::unordered_map<int, std::vector<int>>& bookToGenreMap,
        Vector2 centerPos,
        float dt,
        Node* draggedNode = nullptr);

private:
    float resolveNodeOverlaps(float padding, std::vector<Node>& nodes, const std::vector<bool>& isActive);

    float m_Temperature = 1.0f;

    int m_CoolingHoldFrames = 0;
    static constexpr int m_GridTransitionHoldFrames = 24;
    static constexpr float m_MinTransitionTemperature = 0.35f;
    static constexpr float m_GridCoolingFactor = 0.992f;

    float m_GridTransitionT = 1.0f;
    static constexpr float m_GridTransitionDuration = 0.45f;

    std::unordered_map<int, Vector2> m_GridStartPositions;
    std::unordered_map<int, Vector2> m_TargetPositions;

    float m_LayoutDensityScale = 1.0f;
};