// Implementation of the GraphManager class.
// Core orchestration/state methods kept here; specialized behavior lives in split units.

#include "graphManager.h"
#include "logging.h"

#include <random>

void GraphManager::markNodeIndexCacheDirty() { m_NodeIndexByIdCacheDirty = true; }

void GraphManager::rebuildNodeIndexCache() const {
  if (!m_NodeIndexByIdCacheDirty) {
    return;
  }

  m_NodeIndexByIdCache.clear();
  m_NodeIndexByIdCache.reserve(m_Nodes.size());
  for (size_t i = 0; i < m_Nodes.size(); ++i) {
    m_NodeIndexByIdCache[m_Nodes[i].id] = i;
  }

  m_NodeIndexByIdCacheDirty = false;
}

std::string GraphManager::getGenreNameByNodeId(int nodeId) const {
  auto opt = getGenreByNodeId(nodeId);
  return opt.value_or("Unknown");
}

std::unordered_map<int, NodePosition> GraphManager::exportPositions() const {
  std::unordered_map<int, NodePosition> posMap;
  for (const auto &node : m_Nodes) {
    posMap[node.id] = {node.position.x, node.position.y, node.locked};
  }
  return posMap;
}

bool GraphManager::applyLoadedPositions(
    const std::unordered_map<int, NodePosition> &loadedPos) {
  if (loadedPos.empty()) {
    return false;
  }

  bool allNodesHaveSavedPositions = true;

  for (auto &node : m_Nodes) {
    auto it = loadedPos.find(node.id);
    if (it == loadedPos.end()) {
      allNodesHaveSavedPositions = false;
      continue;
    }

    const NodePosition &saved = it->second;
    node.position = {saved.x, saved.y};
    node.locked = saved.locked;

    if (node.type == NodeType::Genre) {
      updateGenrePosition(node.id, node.position);
    }
  }

  return allNodesHaveSavedPositions;
}

std::optional<std::string> GraphManager::getGenreByNodeId(int nodeId) const {
  for (const auto &[name, info] : m_Genres) {
    if (info.nodeId == nodeId)
      return name;
  }
  return std::nullopt;
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
  markNodeIndexCacheDirty();
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
      continue;
    }

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

  markNodeIndexCacheDirty();
}

GraphManager::GraphManager(const BookManager &bm, ConnectionManager &cm,
                           Vector2 canvasSize, TextRenderer *tr)
    : m_BookManager(bm), m_ConnectionManager(cm), m_NodeRenderer(bm, tr),
      m_CanvasSize(canvasSize) {}

void GraphManager::initializePositions(bool preserveExistingPositions) {
  std::unordered_map<int, Vector2> oldBookPos;
  std::unordered_map<std::string, Vector2> oldGenrePos;

  // Preserve existing positions so rebuilding connections does not always reshuffle nodes.
  if (preserveExistingPositions) {
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
  }

  m_ConnectionManager.updateConnections(m_BookManager.getBooks());
  resetNodeState();
  placeGenreNodes(m_ConnectionManager.getExistingGenres(),
                  m_ConnectionManager.getGenreIdMap(), oldGenrePos);
  placeBookNodes(oldBookPos);
}

void GraphManager::updateGenrePosition(int nodeId, Vector2 newPos) {
  for (auto &[genre, info] : m_Genres) {
    if (info.nodeId == nodeId) {
      info.position = newPos;
      break;
    }
  }
}
