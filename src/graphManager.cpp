#include "graphManager.h"
#include "logging.h"

std::string GraphManager::getGenreNameByNodeId(int nodeId) const {
  auto opt = getGenreByNodeId(nodeId);
  return opt.value_or("Unknown");
}

bool GraphManager::TryGrabNodeAt(Vector2 mousePos, bool isShiftPressed) {
  for (auto &node : m_Nodes) {
    if (Vector2Distance(mousePos, node.position) <= node.radius) {
      node.isDragged = true;
      if (isShiftPressed && node.type != NodeType::Genre) {
        node.locked = true;
      }
      // ZM�NA ZDE:
      setDraggedNode(node.id);
      return true;
    }
  }
  return false;
}

void GraphManager::releaseDraggedNode() {
  Node *dragged = getDraggedNode(); // Pou�ije na�i novou metodu
  if (dragged) {
    dragged->isDragged = false;
    m_DraggedNodeId = -1; // Vyresetujeme ID
  }
}

bool GraphManager::updateDraggedNodePosition(Vector2 mousePos) {
  Node *dragged = getDraggedNode(); // Pou�ije na�i novou metodu
  if (dragged) {
    dragged->position = mousePos;
    if (dragged->type == NodeType::Genre) {
      updateGenrePosition(dragged->id, mousePos);
    }
    return true;
  }
  return false;
}

bool GraphManager::tryUnlockNodeAt(Vector2 mousePos) {
  Node *node = getNodeAtPosition(mousePos);
  if (node && node->locked) {
    node->locked = false;
    return true;
  }
  return false;
}

std::unordered_map<int, NodePosition> GraphManager::exportPositions() const {
  std::unordered_map<int, NodePosition> posMap;
  for (const auto &node : m_Nodes) {
    if (node.type == NodeType::Book) {
      posMap[node.id] = {node.position.x, node.position.y, node.locked};
    }
  }
  return posMap;
}

void GraphManager::applyLoadedPositions(
    const std::unordered_map<int, NodePosition> &loadedPos) {
  if (loadedPos.empty())
    return;

  for (auto &node : m_Nodes) {
    if (node.type == NodeType::Book && loadedPos.count(node.id)) {
      node.position.x = loadedPos.at(node.id).x;
      node.position.y = loadedPos.at(node.id).y;

      node.locked = loadedPos.at(node.id).locked;
    }
  }
}

std::optional<std::string> GraphManager::getGenreByNodeId(int nodeId) const {
  for (const auto &[name, info] : m_Genres) {
    if (info.nodeId == nodeId)
      return name;
  }
  return std::nullopt;
}

Node *GraphManager::getNodeAtPosition(Vector2 mousePos) {
  for (auto &node : m_Nodes) {
    float dist = Vector2Distance(mousePos, node.position);
    if (dist <= node.radius) {
      return &node;
    }
  }
  return nullptr;
}

void GraphManager::updatePhysics(float dt) {

  if (!m_IsPhysicsActive)
    return;

  if (m_LayoutMode == LayoutMode::Grid) {
    m_IsPhysicsActive = m_GraphLayout.updateLerp(m_Nodes, dt);
    return;
  }

  std::unordered_map<int, std::vector<int>> bookToGenreMap;

  // Vytvo��me si mapov�n� (Kniha -> Jej� ��nry)
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

void GraphManager::drawEdges(float zoom, const Rectangle &viewRect) {

  m_NodeRenderer.drawEdges(m_ConnectionManager, m_Nodes, viewRect, zoom);
}

int GraphManager::getNumOfConnectedBooks(int genreNodeId) const {
  return m_ConnectionManager.getConnectedBooksCount(genreNodeId);
}

Node *GraphManager::getNodeById(int id) {
  if (id == -1)
    return nullptr;
  auto it = std::find_if(m_Nodes.begin(), m_Nodes.end(),
                         [id](const Node &n) { return n.id == id; });
  return (it != m_Nodes.end()) ? &(*it) : nullptr;
}

void GraphManager::resetNodeState() {
  m_Nodes.clear();
  m_Genres.clear();
  m_GenreIdBase = -1;
}

float GraphManager::calculateCircleRadius(int nodeCount,
                                          float minRadius) const {
  if (nodeCount == 0)
    return minRadius;
  return std::max(minRadius, (nodeCount * 2.0f * GraphConfig::BaseNodeRadius) /
                                 (2.0f * PI));
}

void GraphManager::placeGenreNodes(
    const std::unordered_set<std::string> &genres,
    const std::unordered_map<std::string, int> &genreIdMap,
    const std::unordered_map<std::string, Vector2> &oldPositions) {
  Vector2 canvasCenter = {m_CanvasSize.x / 2.0f, m_CanvasSize.y / 2.0f};

  float genreRadius =
      GraphConfig::BaseNodeRadius * GraphConfig::GenreRadiusMultiplier;

  // Calculate the radius of the large circle on which genres will be placed
  float genreCircleRadius =
      calculateCircleRadius((int)genres.size(), m_CanvasSize.x / 3.0f);

  int i = 0;
  for (const auto &genreStr : genres) {

    float angle = 2.0f * PI * i++ / genres.size();
    Vector2 pos = {canvasCenter.x + genreCircleRadius * cos(angle),
                   canvasCenter.y + genreCircleRadius * sin(angle)};

    if (oldPositions.find(genreStr) != oldPositions.end()) {
      pos = oldPositions.at(genreStr);
    }

    auto it = genreIdMap.find(genreStr);
    if (it == genreIdMap.end()) {
      Log::Warn("Genre ID not found for genre: " + genreStr);
      continue;
    }
    int genreId = it->second;

    m_Nodes.push_back({genreId, NodeType::Genre, pos, genreRadius});
    m_Genres[genreStr] = {genreId, pos};
  }
}

void GraphManager::placeBookNodes(
    const std::unordered_map<int, Vector2> &oldPositions) {
  Vector2 canvasCenter = {m_CanvasSize.x / 2.0f, m_CanvasSize.y / 2.0f};
  float nodeRadius = GraphConfig::BaseNodeRadius;

  static std::default_random_engine rng(std::random_device{}());
  std::uniform_real_distribution<float> distAngle(0.0f, 2.0f * PI);

  for (const auto &book : m_BookManager.getBooks()) {

    if (oldPositions.find(book.getId()) != oldPositions.end()) {
      m_Nodes.push_back({book.getId(), NodeType::Book,
                         oldPositions.at(book.getId()), nodeRadius});
      continue; // Skip the rest of the calculation logic for this book
    }

    // New position
    Vector2 avgPos = {};
    int count = 0;

    for (const auto &genreStr : book.getGenres()) {
      auto it = m_Genres.find(genreStr);
      if (it != m_Genres.end()) {
        avgPos = Vector2Add(avgPos, it->second.position);
        count++;
      }
    }

    avgPos = (count > 0) ? Vector2Scale(avgPos, 1.0f / count) : canvasCenter;

    float angle = distAngle(rng);
    Vector2 pos = {avgPos.x + GraphConfig::BookSpawnOffset * cos(angle),
                   avgPos.y + GraphConfig::BookSpawnOffset * sin(angle)};

    m_Nodes.push_back({book.getId(), NodeType::Book, pos, nodeRadius});
  }
}

bool GraphManager::isNodeVisible(const Node &node, const Rectangle &viewRect) {
  return !(node.position.x + node.radius < viewRect.x ||
           node.position.x - node.radius > viewRect.x + viewRect.width ||
           node.position.y + node.radius < viewRect.y ||
           node.position.y - node.radius > viewRect.y + viewRect.height);
}

GraphManager::GraphManager(const BookManager &bm, ConnectionManager &cm,
                           Vector2 canvasSize, TextRenderer *tr)
    : m_BookManager(bm), m_ConnectionManager(cm), m_NodeRenderer(bm, tr),
      m_CanvasSize(canvasSize) {}

void GraphManager::initializePositions() {
  std::unordered_map<int, Vector2> oldBookPos;
  std::unordered_map<std::string, Vector2> oldGenrePos;

  for (const auto &node : m_Nodes) {
    if (node.type == NodeType::Book) {
      oldBookPos[node.id] = node.position;
    } else if (node.type == NodeType::Genre) {
      auto nameOpt = getGenreByNodeId(node.id);
      if (nameOpt.has_value()) {
        oldGenrePos[nameOpt.value()] = node.position;
      }
    }
  }

  m_ConnectionManager.updateConnections(m_BookManager.getBooks());
  resetNodeState();
  placeGenreNodes(m_ConnectionManager.getExistingGenres(),
                  m_ConnectionManager.getGenreIdMap(), oldGenrePos);
  placeBookNodes(oldBookPos);
}

void GraphManager::removeNodeById(int id) {
  auto removeSingleNode = [&](int targetId) {
    m_Nodes.erase(std::remove_if(m_Nodes.begin(), m_Nodes.end(),
                                 [targetId](const Node &node) {
                                   return node.id == targetId;
                                 }),
                  m_Nodes.end());

    m_ConnectionManager.removeEdgesConnectedToNode(targetId);

    for (auto it = m_Genres.begin(); it != m_Genres.end();) {
      if (it->second.nodeId == targetId) {
        it = m_Genres.erase(it);
      } else {
        ++it;
      }
    }

    // OPRAVA: Kontrola ta�en�ho uzlu p�es ID
    if (m_DraggedNodeId == targetId) {
      m_DraggedNodeId = -1;
    }
  };

  removeSingleNode(id);

  std::vector<int> emptyGenres;
  for (const auto &node : m_Nodes) {
    if (node.type == NodeType::Genre) {
      if (getNumOfConnectedBooks(node.id) == 0) {
        emptyGenres.push_back(node.id);
      }
    }
  }

  for (int genreId : emptyGenres) {
    Log::Info("Auto-removing empty Genre Node ID: " + std::to_string(genreId));
    removeSingleNode(genreId);
  }
}

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

std::string statusEnumToString(Status s) {
  switch (s) {
  case Status::ToRead:
    return "to read";
  case Status::Reading:
    return "reading";
  case Status::Read:
    return "read";
  default:
    return "";
  }
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

      // Check Genre (Hide book if *any* of its genres are hidden)
      // Alternatively: Hide only if *all* genres are hidden.
      // Below implements "Hide if any genre matches the blocklist"
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

  m_NodeRenderer.drawNode(node, zoom, m_Genres);
}

void GraphManager::updateGenrePosition(int nodeId, Vector2 newPos) {
  // Update position inside m_Genres for the genre that matches nodeId
  for (auto &[genre, info] : m_Genres) {
    if (info.nodeId == nodeId) {
      info.position = newPos;
      break;
    }
  }
}

void GraphManager::setLayoutMode(LayoutMode mode) {
  if (m_LayoutMode == mode) {
    return;
  }

  m_LayoutMode = mode;

  if (m_LayoutMode == LayoutMode::Grid) {
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
    if (!m_PreGridPositions.empty()) {
      for (auto &node : m_Nodes) {
        auto it = m_PreGridPositions.find(node.id);
        if (it != m_PreGridPositions.end()) {
          node.position = it->second;
          if (node.type == NodeType::Genre) {
            updateGenrePosition(node.id, node.position);
          }
        }
      }
    }

    wakeUpPhysics();
  }
}
