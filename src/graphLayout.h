#pragma once
#include "nodeRenderer.h"
#include <raymath.h>
#include <unordered_map>
#include <vector>

class GraphLayout
{
public:
    GraphLayout() = default;


    void calculateGridLayout(std::vector<Node>& nodes, Vector2 centerPos);
    bool updateLerp(std::vector<Node>& nodes);

    // Hlavní fyzikální smyèka: aplikuje pøitalivost, odpuzování a gravitaci
    bool updatePhysics(
        std::vector<Node>& nodes,
        const std::unordered_map<int, std::vector<int>>& bookToGenreMap,
        Vector2 centerPos,
        float dt);

private:
    // Tvrdá kolize - zajišuje, aby se bubliny fyzicky nepøekrıvaly, kdy do sebe narazí
    float resolveNodeOverlaps(float padding, std::vector<Node>& nodes);

    std::unordered_map<int, Vector2> m_TargetPositions;
};