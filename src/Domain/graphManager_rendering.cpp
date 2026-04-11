#include "graphManager.h"
#include "colors.h"
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

    if (node.type == NodeType::Book && m_MultiSelectedBookIds.contains(node.id)) {
      DrawCircleLines(static_cast<int>(node.position.x), static_cast<int>(node.position.y), node.radius + 10.0f, Fade(NookCol::UI_ACCENT, 0.95f));
      DrawCircleLines(static_cast<int>(node.position.x), static_cast<int>(node.position.y), node.radius + 12.0f, Fade(NookCol::UI_ACCENT_SOFT, 0.8f));
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
