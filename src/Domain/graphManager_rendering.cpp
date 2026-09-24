#include "graphManager.h"
#include "colors.h"

#include <algorithm>
const Rectangle GraphManager::getCameraViewRect(const Camera2D &camera,
                                                Vector2 screenDimensions) {
  float left = camera.target.x -
               (static_cast<float>(screenDimensions.x) / 2) / camera.zoom;
  float top = camera.target.y -
              (static_cast<float>(screenDimensions.y) / 2) / camera.zoom;
  float width = screenDimensions.x / camera.zoom;
  float height = screenDimensions.y / camera.zoom;
  return {left, top, width, height};
}

bool GraphManager::isNodeVisible(const Node &node, const Rectangle &viewRect) {
  return !(node.position.x + node.radius < viewRect.x ||
           node.position.x - node.radius > viewRect.x + viewRect.width ||
           node.position.y + node.radius < viewRect.y ||
           node.position.y - node.radius > viewRect.y + viewRect.height);
}

void GraphManager::drawEdges(float zoom, const Rectangle &viewRect) {
  rebuildNodeIndexCache();

  if (!m_SearchFilter.isActive()) {
    m_NodeRenderer.drawEdges(m_ConnectionManager, m_Nodes, viewRect, zoom,
                             nullptr, &m_NodeIndexByIdCache);
    return;
  }

  std::unordered_map<int, bool> dimmedNodes;
  dimmedNodes.reserve(m_Nodes.size());

  for (const auto &node : m_Nodes) {
    bool isDimmed = false;
    if (node.type == NodeType::Book) {
      const Book *b = m_BookManager.findBookById(node.id);
      isDimmed = !m_SearchFilter.matchesBook(b);
    } else if (node.type == NodeType::Genre) {
      std::string genreName = getGenreNameByNodeId(node.id);
      isDimmed = !m_SearchFilter.matchesGenre(genreName);
    }
    dimmedNodes[node.id] = isDimmed;
  }

  m_NodeRenderer.drawEdges(m_ConnectionManager, m_Nodes, viewRect, zoom,
                           &dimmedNodes, &m_NodeIndexByIdCache);
}

void GraphManager::drawNodes(float zoom, const Rectangle &viewRect) {
  bool searchActive = m_SearchFilter.isActive();
  const bool valueGridMode = (m_LayoutMode == LayoutMode::ValueGrid);

  const Node *topBookNode = nullptr;
  const Node *bottomBookNode = nullptr;
  int bottomRank = 0;

  for (const auto &node : m_Nodes) {
    if (!isNodeVisible(node, viewRect))
      continue;
    if (!node.visible)
      continue;

    bool isDimmed = false;

    if (searchActive) {
      if (node.type == NodeType::Book) {
        const Book *b = m_BookManager.findBookById(node.id);
        isDimmed = !m_SearchFilter.matchesBook(b);
      } else if (node.type == NodeType::Genre) {
        std::string genreName = getGenreNameByNodeId(node.id);
        isDimmed = !m_SearchFilter.matchesGenre(genreName);
      }
    }

    m_NodeRenderer.drawNode(node, zoom, m_Genres, isDimmed);

    if (valueGridMode && node.type == NodeType::Book) {
      const auto rankIt = m_ValueGridRankByBookId.find(node.id);
      if (rankIt != m_ValueGridRankByBookId.end()) {
        const int rank = rankIt->second;
        const float rankFontSize = std::clamp(16.0f / std::max(zoom, 0.08f), 12.0f, 48.0f);

        const Vector2 labelPos = {node.position.x - node.radius * 0.9f,
                                  node.position.y - node.radius * 1.25f};
        if (TextRenderer *tr = m_NodeRenderer.getTextRenderer()) {
          tr->DrawSimpleText("#" + std::to_string(rank), labelPos, rankFontSize,
                             Fade(NookCol::UI_TEXT, 0.95f));
        }

        if (rank == 1) {
          topBookNode = &node;
        }
        if (rank > bottomRank) {
          bottomRank = rank;
          bottomBookNode = &node;
        }
      }
    }

    if (node.type == NodeType::Book && m_MultiSelectedBookIds.contains(node.id)) {
      DrawCircleLines(static_cast<int>(node.position.x), static_cast<int>(node.position.y), node.radius + 10.0f, Fade(NookCol::UI_ACCENT, 0.95f));
      DrawCircleLines(static_cast<int>(node.position.x), static_cast<int>(node.position.y), node.radius + 12.0f, Fade(NookCol::UI_ACCENT_SOFT, 0.8f));
    }
  }

  if (valueGridMode) {
    const float hintFontSize = std::clamp(18.0f / std::max(zoom, 0.08f), 14.0f, 54.0f);
    TextRenderer *tr = m_NodeRenderer.getTextRenderer();
    if (topBookNode) {
      const Vector2 topPos = {
          topBookNode->position.x - topBookNode->radius * 0.5f,
          topBookNode->position.y - topBookNode->radius * 2.0f};
      if (tr) {
        tr->DrawSimpleText("TOP", topPos, hintFontSize,
                           Fade(NookCol::UI_ACCENT, 0.95f));
      }
    }
    if (bottomBookNode) {
      const Vector2 bottomPos = {
          bottomBookNode->position.x - bottomBookNode->radius * 0.9f,
          bottomBookNode->position.y + bottomBookNode->radius * 1.4f};
      if (tr) {
        tr->DrawSimpleText("BOTTOM", bottomPos, hintFontSize,
                           Fade(NookCol::UI_TEXT_MUTED, 0.95f));
      }
    }
  }
}

void GraphManager::recalculateVisibility() {
  for (auto &node : m_Nodes) {

    node.visible = true;

    if (node.type == NodeType::Book) {
      const Book *b = m_BookManager.findBookById(node.id);
      if (!b)
        continue;

      if (m_HiddenStatuses.count(b->getStatus())) {
        node.visible = false;
        continue;
      }

      // Book visibility is conjunction of status filter and genre filter.
      for (const auto &g : b->getGenres()) {
        if (m_HiddenGenres.count(g)) {
          node.visible = false;
          break;
        }
      }
    } else if (node.type == NodeType::Genre) {

      std::string genreName = getGenreNameByNodeId(node.id);
      if (m_HiddenGenres.count(genreName)) {
        node.visible = false;
      }
    }
  }
}

void GraphManager::drawNode(const Node &node, float zoom) {

  m_NodeRenderer.drawNode(node, zoom, m_Genres, false);
}
