#include "graphManager.h"

namespace {
Rectangle NormalizeRectangle(Rectangle rect) {
  if (rect.width < 0.0f) {
    rect.x += rect.width;
    rect.width = -rect.width;
  }
  if (rect.height < 0.0f) {
    rect.y += rect.height;
    rect.height = -rect.height;
  }
  return rect;
}
}

void GraphManager::SetSelectionRectangleWorld(Vector2 worldStart, Vector2 worldEnd) {
  m_SelectionRectWorld = NormalizeRectangle({
      worldStart.x,
      worldStart.y,
      worldEnd.x - worldStart.x,
      worldEnd.y - worldStart.y
      });
  m_SelectionRectActive = true;
}

void GraphManager::ClearSelectionRectangleWorld() {
  m_SelectionRectActive = false;
  m_SelectionRectWorld = { 0, 0, 0, 0 };
}

void GraphManager::DrawSelectionRectangleWorld() const {
  if (!m_SelectionRectActive) {
    return;
  }

  DrawRectangleRec(m_SelectionRectWorld, Fade(NookCol::UI_ACCENT_SOFT, 0.18f));
  DrawRectangleLinesEx(m_SelectionRectWorld, 2.0f, Fade(NookCol::UI_ACCENT, 0.95f));
}

std::vector<int> GraphManager::GetBookNodeIdsInWorldRect(const Rectangle& worldRect, bool onlyVisible) const {
  const Rectangle rect = NormalizeRectangle(worldRect);
  std::vector<int> ids;
  ids.reserve(m_Nodes.size());

  for (const Node& node : m_Nodes) {
    if (node.type != NodeType::Book) {
      continue;
    }
    if (onlyVisible && !node.visible) {
      continue;
    }

    if (CheckCollisionPointRec(node.position, rect)) {
      ids.push_back(node.id);
    }
  }

  return ids;
}

std::vector<int> GraphManager::GetMultiSelectedBookIds() const {
  std::vector<int> ids;
  ids.reserve(m_MultiSelectedBookIds.size());
  for (int id : m_MultiSelectedBookIds) {
    ids.push_back(id);
  }
  return ids;
}

void GraphManager::SetMultiSelectedBookIds(const std::vector<int>& bookIds) {
  m_MultiSelectedBookIds.clear();
  for (int id : bookIds) {
    m_MultiSelectedBookIds.insert(id);
  }
}

void GraphManager::AddMultiSelectedBookIds(const std::vector<int>& bookIds) {
  for (int id : bookIds) {
    m_MultiSelectedBookIds.insert(id);
  }
}

void GraphManager::ClearMultiSelectedBookIds() {
  m_MultiSelectedBookIds.clear();
}

bool GraphManager::TranslateMultiSelectedBookNodes(Vector2 delta) {
  if (m_MultiSelectedBookIds.empty()) {
    return false;
  }

  bool movedAny = false;
  for (int id : m_MultiSelectedBookIds) {
    Node* node = getNodeById(id);
    if (!node || node->type != NodeType::Book) {
      continue;
    }

    node->position = Vector2Add(node->position, delta);
    movedAny = true;
  }

  return movedAny;
}

bool GraphManager::HasSignificantNodeOverlaps(float overlapFactor) const {
  if (m_Nodes.size() < 2) {
    return false;
  }

  const float clampedFactor = std::clamp(overlapFactor, 0.1f, 1.0f);

  for (size_t i = 0; i < m_Nodes.size(); ++i) {
    const Node& a = m_Nodes[i];
    if (!a.visible) continue;

    for (size_t j = i + 1; j < m_Nodes.size(); ++j) {
      const Node& b = m_Nodes[j];
      if (!b.visible) continue;

      const float minDist = (a.radius + b.radius) * clampedFactor;
      if (Vector2Distance(a.position, b.position) < minDist) {
        return true;
      }
    }
  }

  return false;
}
