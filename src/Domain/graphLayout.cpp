
// Implementation of the GraphLayout class.
// Implements physics simulation and grid layout algorithms for node positioning.


#include "graphLayout.h"
#include <algorithm>

namespace {
float SmoothStep01(float t) {
    const float x = std::clamp(t, 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}

float Hash01(int v) {
    unsigned int x = (unsigned int)v;
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return (float)(x & 0xFFFFU) / 65535.0f;
}
}

void GraphLayout::setLayoutDensityScale(float scale)
{
    m_LayoutDensityScale = std::clamp(scale, 0.3f, 1.6f);
}


void GraphLayout::calculateGridLayout(
    std::vector<Node>& nodes,
    Vector2 centerPos,
    const std::unordered_map<int, std::vector<int>>& bookToGenreMap)
{
    m_TargetPositions.clear();

    std::vector<Node*> genres;
    std::vector<Node*> books;

   
    for (auto& n : nodes) {
        if (n.type == NodeType::Genre) genres.push_back(&n);
        else books.push_back(&n);
    }

    float spacingX = 280.0f; 
    float spacingY = 300.0f; 

   
    float startX = centerPos.x - 600.0f;
    
    float startY = centerPos.y - (genres.size() * spacingY) / 2.0f;

    std::unordered_set<int> placedBooks; 
    int row = 0;

    
    for (auto* g : genres) {
        
        m_TargetPositions[g->id] = { startX, startY + row * spacingY };

        int col = 1; 

        for (auto* b : books) {
       
            if (placedBooks.count(b->id)) continue;

            auto it = bookToGenreMap.find(b->id);
            if (it != bookToGenreMap.end()) {
                const auto& bookGenres = it->second;
              
                if (std::find(bookGenres.begin(), bookGenres.end(), g->id) != bookGenres.end()) {
                  
                    m_TargetPositions[b->id] = { startX + col * spacingX, startY + row * spacingY };
                    placedBooks.insert(b->id);
                    col++;
                }
            }
        }
        row++; 
    }

   
    int orphansCol = 0;
    for (auto* b : books) {
        if (!placedBooks.count(b->id)) {
            
            m_TargetPositions[b->id] = { startX + orphansCol * spacingX, startY + row * spacingY };
            orphansCol++;
        }
    }

    m_GridStartPositions.clear();
    for (const auto& node : nodes) {
        m_GridStartPositions[node.id] = node.position;
    }
    m_GridTransitionT = 0.0f;

    m_Temperature = std::max(m_Temperature, m_MinTransitionTemperature);
    m_CoolingHoldFrames = m_GridTransitionHoldFrames;
}

bool GraphLayout::updateLerp(std::vector<Node>& nodes, float dt) {
    bool isMoving = false;

    // Higher temperature makes grid transitions complete faster right after layout switches.
    const float tempBoost = std::clamp(m_Temperature, 0.0f, 1.0f);
    const float transitionDuration = m_GridTransitionDuration * (1.15f - 0.35f * tempBoost);
    m_GridTransitionT = std::min(1.0f, m_GridTransitionT + (dt / std::max(transitionDuration, 0.01f)));
    const float alpha = SmoothStep01(m_GridTransitionT);

    for (auto& n : nodes) {
        if (!n.isDragged && m_TargetPositions.count(n.id)) {
            const Vector2 target = m_TargetPositions[n.id];
            const Vector2 start = m_GridStartPositions.count(n.id) ? m_GridStartPositions[n.id] : n.position;
            float dist = Vector2Distance(n.position, target);

            if (m_GridTransitionT < 1.0f) {
                n.position = Vector2Lerp(start, target, alpha);
                if (Vector2Distance(n.position, target) > 0.5f) {
                    isMoving = true;
                }
            }
            else if (dist > 0.5f) {
                n.position = target;
                isMoving = true;
            }
            else {
                n.position = target; 
            }
        }
    }

    if (m_CoolingHoldFrames > 0) {
        --m_CoolingHoldFrames;
    }
    else {
        m_Temperature *= m_GridCoolingFactor;
        if (m_Temperature < 0.0f) {
            m_Temperature = 0.0f;
        }
    }

    return m_GridTransitionT < 1.0f || isMoving;
}

bool GraphLayout::updatePhysics(
    std::vector<Node>& nodes,
    const std::unordered_map<int, std::vector<int>>& bookToGenreMap,
    Vector2 centerPos,
    float dt,
    Node* draggedNode)
{
    std::vector<Vector2> displacements(nodes.size(), { 0.0f, 0.0f });
    std::vector<bool> isActive(nodes.size(), true);

    if (draggedNode != nullptr) {
        const float activeRadius = 1200.0f;
        std::fill(isActive.begin(), isActive.end(), false);
        for (size_t i = 0; i < nodes.size(); ++i) {
            if (Vector2Distance(nodes[i].position, draggedNode->position) < activeRadius) {
                isActive[i] = true;
            }
        }
    }

    std::vector<size_t> genreIndices;
    std::vector<size_t> bookIndices;
    genreIndices.reserve(nodes.size());
    bookIndices.reserve(nodes.size());

    float maxBookRadius = 1.0f;
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].type == NodeType::Genre) genreIndices.push_back(i);
        else bookIndices.push_back(i);
        maxBookRadius = std::max(maxBookRadius, nodes[i].radius);
    }

    std::sort(genreIndices.begin(), genreIndices.end(), [&nodes](size_t a, size_t b) {
        return nodes[a].id < nodes[b].id;
    });

    const float bookCountF = (float)bookIndices.size();
    const float genreCountF = (float)std::max<size_t>(1, genreIndices.size());
    // 0.0 => small library, 1.0 => very large library.
    const float density = std::clamp((bookCountF - 60.0f) / 260.0f, 0.0f, 1.0f);
    // Make UI slider impact clearly visible: compact side gets much tighter,
    // airy side spreads clusters significantly more than linear scaling.
    const float uiScale = std::clamp(m_LayoutDensityScale, 0.3f, 1.6f);
    const float spacingScale = std::pow(uiScale, 2.2f);
    const float hubScale = std::pow(uiScale, 2.0f);
    const float overlapScale = std::pow(uiScale, 1.8f);

    std::unordered_map<int, size_t> genreIdToNodeIndex;
    genreIdToNodeIndex.reserve(genreIndices.size());
    for (size_t idx : genreIndices) {
        genreIdToNodeIndex[nodes[idx].id] = idx;
    }

    std::unordered_map<int, std::vector<size_t>> clusterMembers;
    clusterMembers.reserve(genreIndices.size());
    std::vector<size_t> orphanBooks;
    orphanBooks.reserve(bookIndices.size());

    for (size_t idx : bookIndices) {
        auto it = bookToGenreMap.find(nodes[idx].id);
        if (it == bookToGenreMap.end() || it->second.empty()) {
            orphanBooks.push_back(idx);
            continue;
        }

        int primaryGenreId = it->second.front();
        for (int gid : it->second) {
            if (gid < primaryGenreId) primaryGenreId = gid;
        }

        if (!genreIdToNodeIndex.count(primaryGenreId)) {
            orphanBooks.push_back(idx);
            continue;
        }

        clusterMembers[primaryGenreId].push_back(idx);
    }

    // Estimate required cluster radius from member count so hub separation can be guaranteed.
    const float slotSpacing = maxBookRadius * (2.70f - 0.65f * density) * spacingScale;
    std::unordered_map<int, float> clusterRadius;
    clusterRadius.reserve(genreIndices.size());
    for (size_t gIdx : genreIndices) {
        const int gid = nodes[gIdx].id;
        const size_t count = clusterMembers.count(gid) ? clusterMembers[gid].size() : 0;

        float radius = maxBookRadius * 2.2f;
        size_t remaining = count;
        int ring = 1;
        while (remaining > 0) {
            const float ringRadius = maxBookRadius * 2.0f + ring * slotSpacing;
            int capacity = std::max(6, (int)std::floor((2.0f * PI * ringRadius) / std::max(1.0f, slotSpacing)));
            const size_t used = std::min(remaining, (size_t)capacity);
            remaining -= used;
            radius = ringRadius + slotSpacing;
            ++ring;
        }

        clusterRadius[gid] = std::max(radius, maxBookRadius * 2.4f);
    }

    // Place genre hubs using deterministic random seeds, then relax with radius-aware separation.
    const float minHubRadius = std::max(140.0f, maxBookRadius * (5.0f - 1.4f * density) * hubScale);
    const float maxHubRadius = std::max(minHubRadius + 120.0f, std::min(centerPos.x, centerPos.y) * (1.48f - 0.38f * density) * hubScale);

    std::vector<Vector2> hubTargets(genreIndices.size(), centerPos);
    std::vector<Vector2> hubAnchors(genreIndices.size(), centerPos);
    std::vector<float> hubRadii(genreIndices.size(), maxBookRadius * 2.0f);

    for (size_t i = 0; i < genreIndices.size(); ++i) {
        const int gid = nodes[genreIndices[i]].id;
        const float a = Hash01(gid * 101 + 17) * 2.0f * PI;
        const float u = Hash01(gid * 131 + 29);
        const float r = minHubRadius + (maxHubRadius - minHubRadius) * std::sqrt(u);

        hubTargets[i] = {
            centerPos.x + r * std::cos(a),
            centerPos.y + r * std::sin(a)
        };
        hubAnchors[i] = hubTargets[i];
        hubRadii[i] = clusterRadius[nodes[genreIndices[i]].id];
    }

    for (int iter = 0; iter < 36; ++iter) {
        std::vector<Vector2> hubDisp(hubTargets.size(), { 0.0f, 0.0f });

        for (size_t i = 0; i < hubTargets.size(); ++i) {
            for (size_t j = i + 1; j < hubTargets.size(); ++j) {
                Vector2 delta = Vector2Subtract(hubTargets[j], hubTargets[i]);
                float dist = Vector2Length(delta);
                if (dist < 0.01f) {
                    const float aa = Hash01((int)(i * 7919 + j * 2971)) * 2.0f * PI;
                    delta = { std::cos(aa), std::sin(aa) };
                    dist = 1.0f;
                }

                const float minDist = hubRadii[i] + hubRadii[j] + maxBookRadius * (5.2f - 1.8f * density) * spacingScale;
                if (dist < minDist) {
                    const float push = (minDist - dist) * 0.52f;
                    Vector2 dir = Vector2Scale(Vector2Normalize(delta), push);
                    hubDisp[i] = Vector2Subtract(hubDisp[i], dir);
                    hubDisp[j] = Vector2Add(hubDisp[j], dir);
                }
            }
        }

        for (size_t i = 0; i < hubTargets.size(); ++i) {
            Vector2 pullToAnchor = Vector2Scale(Vector2Subtract(hubAnchors[i], hubTargets[i]), 0.05f + 0.03f * density);
            Vector2 toCenter = Vector2Subtract(centerPos, hubTargets[i]);
            float centerDist = Vector2Length(toCenter);
            if (centerDist > maxHubRadius) {
                pullToAnchor = Vector2Add(pullToAnchor, Vector2Scale(Vector2Normalize(toCenter), (centerDist - maxHubRadius) * 0.12f));
            }

            hubDisp[i] = Vector2Add(hubDisp[i], pullToAnchor);
            hubTargets[i] = Vector2Add(hubTargets[i], Vector2Scale(hubDisp[i], 0.70f));
        }
    }

    const float genreFollow = std::clamp(dt * 9.0f, 0.0f, 0.38f);
    for (size_t i = 0; i < genreIndices.size(); ++i) {
        const size_t nodeIdx = genreIndices[i];
        if (!isActive[nodeIdx]) continue;

        const Vector2 target = hubTargets[i];

        if (!nodes[nodeIdx].locked && !nodes[nodeIdx].isDragged) {
            Vector2 delta = Vector2Subtract(target, nodes[nodeIdx].position);
            Vector2 move = Vector2Scale(delta, genreFollow);
            const float maxMove = 48.0f;
            const float len = Vector2Length(move);
            if (len > maxMove) move = Vector2Scale(Vector2Normalize(move), maxMove);
            nodes[nodeIdx].position = Vector2Add(nodes[nodeIdx].position, move);
            displacements[nodeIdx] = move;
        }
    }

    // Assign books into non-overlapping ring slots around each hub.
    const float goldenAngle = 2.39996323f;
    const float bookFollow = std::clamp(dt * 8.0f, 0.0f, 0.34f);

    for (auto& [genreId, members] : clusterMembers) {
        auto hubIt = genreIdToNodeIndex.find(genreId);
        if (hubIt == genreIdToNodeIndex.end()) continue;
        const Vector2 hubPos = nodes[hubIt->second].position;

        std::sort(members.begin(), members.end(), [&nodes](size_t a, size_t b) {
            return nodes[a].id < nodes[b].id;
        });

        size_t cursor = 0;
        int ring = 1;
        while (cursor < members.size()) {
            const float ringRadius = maxBookRadius * 2.0f + ring * slotSpacing;
            const int capacity = std::max(6, (int)std::floor((2.0f * PI * ringRadius) / std::max(1.0f, slotSpacing)));
            const size_t used = std::min(members.size() - cursor, (size_t)capacity);

            const float ringOffset = Hash01(genreId + ring * 9176) * 2.0f * PI;
            for (size_t k = 0; k < used; ++k) {
                const size_t bookIdx = members[cursor + k];
                if (!isActive[bookIdx] || nodes[bookIdx].locked || nodes[bookIdx].isDragged) continue;

                const float slotAngle = ringOffset + ((float)k / (float)used) * 2.0f * PI;
                Vector2 target = {
                    hubPos.x + ringRadius * std::cos(slotAngle),
                    hubPos.y + ringRadius * std::sin(slotAngle)
                };

                auto relIt = bookToGenreMap.find(nodes[bookIdx].id);
                if (relIt != bookToGenreMap.end() && relIt->second.size() > 1) {
                    Vector2 centroid = { 0.0f, 0.0f };
                    int cnt = 0;
                    for (int gid : relIt->second) {
                        auto ni = genreIdToNodeIndex.find(gid);
                        if (ni != genreIdToNodeIndex.end()) {
                            centroid = Vector2Add(centroid, nodes[ni->second].position);
                            ++cnt;
                        }
                    }
                    if (cnt > 0) {
                        centroid = Vector2Scale(centroid, 1.0f / (float)cnt);
                        target = Vector2Lerp(target, centroid, 0.20f);
                    }
                }

                Vector2 delta = Vector2Subtract(target, nodes[bookIdx].position);
                Vector2 move = Vector2Scale(delta, bookFollow);
                const float maxMove = 36.0f;
                const float len = Vector2Length(move);
                if (len > maxMove) move = Vector2Scale(Vector2Normalize(move), maxMove);
                nodes[bookIdx].position = Vector2Add(nodes[bookIdx].position, move);
                displacements[bookIdx] = move;
            }

            cursor += used;
            ++ring;
        }
    }

    // Books without genres: independent inner ring so they don't pollute genre clusters.
    if (!orphanBooks.empty()) {
        std::sort(orphanBooks.begin(), orphanBooks.end(), [&nodes](size_t a, size_t b) {
            return nodes[a].id < nodes[b].id;
        });

        const float orphanBase = std::max(90.0f, maxBookRadius * (2.6f - 0.6f * density) * spacingScale);
        for (size_t i = 0; i < orphanBooks.size(); ++i) {
            const size_t bookIdx = orphanBooks[i];
            if (!isActive[bookIdx] || nodes[bookIdx].locked || nodes[bookIdx].isDragged) continue;

            const float layer = std::floor(std::sqrt((float)i + 1.0f));
            const float radius = orphanBase + layer * (slotSpacing * 0.86f);
            const float angle = i * goldenAngle + Hash01(nodes[bookIdx].id) * 0.45f;
            Vector2 target = {
                centerPos.x + radius * std::cos(angle),
                centerPos.y + radius * std::sin(angle)
            };

            Vector2 delta = Vector2Subtract(target, nodes[bookIdx].position);
            Vector2 move = Vector2Scale(delta, bookFollow);
            const float len = Vector2Length(move);
            if (len > 28.0f) move = Vector2Scale(Vector2Normalize(move), 28.0f);
            nodes[bookIdx].position = Vector2Add(nodes[bookIdx].position, move);
            displacements[bookIdx] = move;
        }
    }

    float totalMovement = 0.0f;
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (!isActive[i]) continue;
        totalMovement += Vector2Length(displacements[i]);
    }

    totalMovement += resolveNodeOverlaps(maxBookRadius * (1.45f - 0.38f * density) * overlapScale, nodes, isActive);
    totalMovement += resolveNodeOverlaps(maxBookRadius * (1.10f - 0.30f * density) * overlapScale, nodes, isActive);
    totalMovement += resolveNodeOverlaps(maxBookRadius * (0.78f - 0.20f * density) * overlapScale, nodes, isActive);

    m_Temperature *= (totalMovement < 3.0f) ? 0.93f : 0.985f;

    if (draggedNode != nullptr) {
        return true;
    }

    if (m_Temperature < 0.05f) {
        m_Temperature = 0.0f;
        return false;
    }

    return true;
}


float GraphLayout::resolveNodeOverlaps(float padding, std::vector<Node>& nodes, const std::vector<bool>& isActive)
{
    float maxDisplacement = 30.0f;
    float overlapMovement = 0.0f;

    // Multiple passes reduce stubborn overlaps in dense clusters.
    const int maxPasses = 4;

    for (int pass = 0; pass < maxPasses; ++pass) {
        bool hadOverlap = false;

        for (size_t i = 0; i < nodes.size(); ++i) {
            for (size_t j = i + 1; j < nodes.size(); ++j) {
                if (!isActive[i] && !isActive[j]) continue;

                Node& a = nodes[i];
                Node& b = nodes[j];

                const float inflatedA = a.radius + padding;
                const float inflatedB = b.radius + padding;

                const float aMinX = a.position.x - inflatedA;
                const float aMaxX = a.position.x + inflatedA;
                const float aMinY = a.position.y - inflatedA;
                const float aMaxY = a.position.y + inflatedA;
                const float bMinX = b.position.x - inflatedB;
                const float bMaxX = b.position.x + inflatedB;
                const float bMinY = b.position.y - inflatedB;
                const float bMaxY = b.position.y + inflatedB;

                // AABB broad-phase: aggressively skip non-overlapping pairs.
                if (aMaxX < bMinX || bMaxX < aMinX || aMaxY < bMinY || bMaxY < aMinY) {
                    continue;
                }

                float totalRadius = a.radius + b.radius + padding;
                Vector2 delta = Vector2Subtract(b.position, a.position);
                float dist = Vector2Length(delta);

                if (dist < 0.01f) {
                    float angle = (float)(rand() % 360) * DEG2RAD;
                    delta = { cos(angle), sin(angle) };
                    dist = 1.0f;
                }

                if (dist < totalRadius) {
                    hadOverlap = true;
                    Vector2 direction = Vector2Normalize(delta);
                    float overlap = totalRadius - dist;
                    Vector2 displacement = Vector2Scale(direction, std::min(overlap * 0.85f, maxDisplacement));

                    bool aFixed = a.locked || a.isDragged || !isActive[i];
                    bool bFixed = b.locked || b.isDragged || !isActive[j];

                    float aMove = aFixed ? 0.0f : (bFixed ? 1.0f : 0.5f);
                    float bMove = bFixed ? 0.0f : (aFixed ? 1.0f : 0.5f);

                    if (aMove > 0.0f) {
                        Vector2 moveVec = Vector2Scale(displacement, aMove);
                        a.position = Vector2Subtract(a.position, moveVec);
                        overlapMovement += Vector2Length(moveVec);
                    }
                    if (bMove > 0.0f) {
                        Vector2 moveVec = Vector2Scale(displacement, bMove);
                        b.position = Vector2Add(b.position, moveVec);
                        overlapMovement += Vector2Length(moveVec);
                    }
                }
            }
        }

        if (!hadOverlap) {
            break;
        }

        // Later passes are finer to avoid jitter once heavy overlaps are gone.
        maxDisplacement = std::max(10.0f, maxDisplacement * 0.72f);
    }
    return overlapMovement;
}