#include "graphManager.h"

namespace {
float SmoothStep01(float t) {
  const float x = std::clamp(t, 0.0f, 1.0f);
  return x * x * (3.0f - 2.0f * x);
}
}

void GraphManager::updatePhysics(float dt) {

  if (m_IsRestoringFromGrid) {
    m_RestoreFromGridT = std::min(
        1.0f,
        m_RestoreFromGridT + (dt / std::max(0.01f, m_RestoreFromGridDuration)));

    const float alpha = SmoothStep01(m_RestoreFromGridT);
    bool isMoving = false;

    for (auto &node : m_Nodes) {
      auto startIt = m_RestoreFromGridStartPositions.find(node.id);
      auto targetIt = m_RestoreFromGridTargetPositions.find(node.id);
      if (startIt == m_RestoreFromGridStartPositions.end() ||
          targetIt == m_RestoreFromGridTargetPositions.end()) {
        continue;
      }

      const Vector2 start = startIt->second;
      const Vector2 target = targetIt->second;
      node.position = Vector2Lerp(start, target, alpha);

      if (node.type == NodeType::Genre) {
        updateGenrePosition(node.id, node.position);
      }

      if (Vector2Distance(node.position, target) > 0.5f) {
        isMoving = true;
      }
    }

    if (m_RestoreFromGridT >= 1.0f || !isMoving) {
      for (auto &node : m_Nodes) {
        auto targetIt = m_RestoreFromGridTargetPositions.find(node.id);
        if (targetIt == m_RestoreFromGridTargetPositions.end()) {
          continue;
        }

        node.position = targetIt->second;
        if (node.type == NodeType::Genre) {
          updateGenrePosition(node.id, node.position);
        }
      }

      m_IsRestoringFromGrid = false;
      m_IsPhysicsActive = false;
    }

    return;
  }

  if (!m_IsPhysicsActive)
    return;

  if (m_LayoutMode == LayoutMode::Grid) {
    m_IsPhysicsActive = m_GraphLayout.updateLerp(m_Nodes, dt);
    return;
  }

  std::unordered_map<int, std::vector<int>> bookToGenreMap;

  // Build adjacency once per tick; GraphLayout consumes compact book->genre IDs.
  for (const auto &book : m_BookManager.getBooks()) {
    std::vector<int> connectedGenreNodeIds;
    for (const auto &genreStr : book.getGenres()) {
      auto it = m_Genres.find(genreStr);
      if (it != m_Genres.end()) {
        connectedGenreNodeIds.push_back(it->second.nodeId);
      }
    }
    bookToGenreMap[book.getId()] = connectedGenreNodeIds;
  }

  Vector2 centerPos = {m_CanvasSize.x / 2.0f, m_CanvasSize.y / 2.0f};

  Node *draggedNode = getDraggedNode();

  m_IsPhysicsActive = m_GraphLayout.updatePhysics(m_Nodes, bookToGenreMap,
                                                  centerPos, dt, draggedNode);
}

void GraphManager::setLayoutMode(LayoutMode mode) {
  if (m_LayoutMode == mode) {
    return;
  }

  m_LayoutMode = mode;

  if (m_LayoutMode == LayoutMode::Grid) {
    m_IsRestoringFromGrid = false;
    m_RestoreFromGridStartPositions.clear();
    m_RestoreFromGridTargetPositions.clear();

    // Save organic (physics) coordinates so we can restore them when leaving grid mode.
    m_PreGridPositions.clear();
    for (const auto &node : m_Nodes) {
      m_PreGridPositions[node.id] = node.position;
    }

    std::unordered_map<int, std::vector<int>> bookToGenreMap;
    for (const auto &book : m_BookManager.getBooks()) {
      std::vector<int> connectedGenreNodeIds;
      for (const auto &genreStr : book.getGenres()) {
        auto it = m_Genres.find(genreStr);
        if (it != m_Genres.end()) {
          connectedGenreNodeIds.push_back(it->second.nodeId);
        }
      }
      bookToGenreMap[book.getId()] = connectedGenreNodeIds;
    }

    Vector2 centerPos = {m_CanvasSize.x / 2.0f, m_CanvasSize.y / 2.0f};

    m_GraphLayout.calculateGridLayout(m_Nodes, centerPos, bookToGenreMap);

    m_IsPhysicsActive = true;
  } else {
    // Smoothly return to the pre-grid snapshot instead of rebuilding a fresh layout.
    m_RestoreFromGridStartPositions.clear();
    m_RestoreFromGridTargetPositions.clear();

    for (auto &node : m_Nodes) {
      auto it = m_PreGridPositions.find(node.id);
      if (it == m_PreGridPositions.end()) {
        continue;
      }

      m_RestoreFromGridStartPositions[node.id] = node.position;
      m_RestoreFromGridTargetPositions[node.id] = it->second;
    }

    if (m_RestoreFromGridTargetPositions.empty()) {
      m_IsRestoringFromGrid = false;
      m_IsPhysicsActive = false;
      return;
    }

    m_RestoreFromGridT = 0.0f;
    m_IsRestoringFromGrid = true;
    m_IsPhysicsActive = true;
  }
}
