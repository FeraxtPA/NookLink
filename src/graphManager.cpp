#include "graphManager.h"
#include <iostream>

const std::string GraphManager::getGenreNameByNodeId(int nodeId) const {
    auto opt = getGenreByNodeId(nodeId);
    return opt.value_or("Unknown");
}

const std::optional<std::string> GraphManager::getGenreByNodeId(int nodeId) const {
    for (const auto& [name, info] : m_Genres) {
        if (info.nodeId == nodeId) return name;
    }
    return std::nullopt;
}

Node* GraphManager::getNodeAtPosition(Vector2 mousePos)
{
    for (auto& node : m_Nodes) {
        float dist = Vector2Distance(mousePos, node.position);
        if (dist <= node.radius) {
            return &node;
        }
    }
    return nullptr;
}

void GraphManager::resolveNodeOverlaps(float padding) {


   

    m_GraphLayout.resolveNodeOverlaps(padding, m_Nodes);

    std::unordered_map<int, std::vector<int>> bookToGenreMap;

    for (const auto& book : m_BookManager.getBooks()) {
        std::vector<int> connectedGenreNodeIds;
        for (const auto& genreStr : book.getGenres()) {
            auto it = m_Genres.find(genreStr);
            if (it != m_Genres.end()) {
                connectedGenreNodeIds.push_back(it->second.nodeId);
            }
        }
        bookToGenreMap[book.getId()] = connectedGenreNodeIds;
    }

    std::unordered_map<int, int> genreToBookCount;
    for (const auto& [bookId, genreIds] : bookToGenreMap) {
        for (int genreId : genreIds) {
            genreToBookCount[genreId]++;
        }
    }

    m_GraphLayout.applySpringConstraints(m_Nodes, bookToGenreMap, genreToBookCount, 750.0f, 0.5f);

  
}



void GraphManager::drawEdges(float zoom, const Rectangle& viewRect) {

    m_NodeRenderer.drawEdges(m_ConnectionManager, m_Nodes, viewRect, zoom);
}

int GraphManager::getNumOfConnectedBooks(int genreNodeId) const
{
    int counter = 0;
    for (const auto& edge : m_ConnectionManager.getEdges()) {
		if (edge.type == EdgeType::BookToGenre && edge.toId == genreNodeId) {
			counter++;
		}
	}
    return counter;
}


void GraphManager::resetNodeState() {
    m_Nodes.clear();
    m_Genres.clear();
    m_GenreIdBase = -1;
}

float GraphManager::calculateCircleRadius(int nodeCount, float minRadius) const
{
    if (nodeCount == 0) return minRadius;
    return std::max(minRadius, (nodeCount * 2.0f * 100.0f) / (2.0f * PI)); 
}

void GraphManager::placeGenreNodes(const std::unordered_set<std::string>& genres, const std::unordered_map<std::string, int>& genreIdMap) {
    Vector2 canvasCenter = { m_CanvasSize.x / 2.0f, m_CanvasSize.y / 2.0f };
    float nodeRadius = 100.0f;

    // Calculate the radius of the large circle on which genres will be placed
    float genreCircleRadius = calculateCircleRadius((int)genres.size(), m_CanvasSize.x / 3.0f);

    int i = 0;
    for (const auto& genreStr : genres) {
        // Distribute nodes evenly around the circle
        float angle = 2.0f * PI * i++ / genres.size();
        Vector2 pos = {
            canvasCenter.x + genreCircleRadius * cos(angle),
            canvasCenter.y + genreCircleRadius * sin(angle)
        };

        // Get the existing ID from genreIdMap (created in ConnectionManager)
        auto it = genreIdMap.find(genreStr);
        if (it == genreIdMap.end()) {
            std::cerr << "Warning: Genre ID not found for genre " << genreStr << "\n";
            continue;
        }
        int genreId = it->second;

        // Make genre nodes slightly larger than book nodes
        float genreRadius = nodeRadius * 1.5f;

        // Add the node to the simulation
        m_Nodes.push_back({ genreId, NodeType::Genre, pos, genreRadius });

        // Store info for the renderer/layout
        m_Genres[genreStr] = { genreId, pos };
    }
}


void GraphManager::placeBookNodes() {
    Vector2 canvasCenter = { m_CanvasSize.x / 2.0f, m_CanvasSize.y / 2.0f };
    float nodeRadius = 100.0f;
    float offsetDistance = 250.0f;

    static std::default_random_engine rng(std::random_device{}());
    std::uniform_real_distribution<float> distAngle(0.0f, 2.0f * PI);

    for (const auto& book : m_BookManager.getBooks()) {
        Vector2 avgPos = {};
        int count = 0;

        // Iterate over the book's genres (now strings)
        for (const auto& genreStr : book.getGenres()) {
            // Find the genre node position in the map (keys are now strings)
            auto it = m_Genres.find(genreStr);
            if (it != m_Genres.end()) {
                avgPos = Vector2Add(avgPos, it->second.position);
                count++;
            }
        }

        // If the book has genres, place it near them; otherwise, place it near the center
        avgPos = (count > 0) ? Vector2Scale(avgPos, 1.0f / count) : canvasCenter;

        // Add some random offset so nodes don't stack perfectly on top of each other
        float angle = distAngle(rng);
        Vector2 pos = {
            avgPos.x + offsetDistance * cos(angle),
            avgPos.y + offsetDistance * sin(angle)
        };

        m_Nodes.push_back({ book.getId(), NodeType::Book, pos, nodeRadius });
    }
}


bool GraphManager::isNodeVisible(const Node& node, const Rectangle& viewRect) {
    return !(node.position.x + node.radius < viewRect.x ||
        node.position.x - node.radius > viewRect.x + viewRect.width ||
        node.position.y + node.radius < viewRect.y ||
        node.position.y - node.radius > viewRect.y + viewRect.height);
}

GraphManager::GraphManager(const BookManager& bm, ConnectionManager& cm, Vector2 canvasSize, TextRenderer* tr)
    : m_BookManager(bm), m_ConnectionManager(cm), m_NodeRenderer(bm, tr), m_CanvasSize(canvasSize)
{
}

void GraphManager::initializePositions()
{
    m_ConnectionManager.updateConnections(m_BookManager.getBooks());

    resetNodeState();

    placeGenreNodes(m_ConnectionManager.getExistingGenres(), m_ConnectionManager.getGenreIdMap());

    placeBookNodes();

    for (auto& node : m_Nodes) {
        m_NodeIdMap[node.id] = &node;
    }

}

void GraphManager::removeNodeById(int id) {
  
    m_Nodes.erase(std::remove_if(m_Nodes.begin(), m_Nodes.end(),
        [id](const Node& node) { return node.id == id; }), m_Nodes.end());

    m_ConnectionManager.removeEdgesConnectedToNode(id);


    // Find if the id corresponds to any Genre and remove it
    for (auto it = m_Genres.begin(); it != m_Genres.end(); ++it) {
        if (it->second.nodeId == id) {
            m_Genres.erase(it);
            break;
        }
    }

    m_NodeIdMap.erase(id);

    if (draggedNode && draggedNode->id == id) {
        draggedNode = nullptr;
    }
}



const Rectangle GraphManager::getCameraViewRect(const Camera2D& camera, Vector2 screenDimensions) {
    float left = camera.target.x - (static_cast<float>(screenDimensions.x) / 2) / camera.zoom;
    float top = camera.target.y - (static_cast<float>(screenDimensions.y) / 2) / camera.zoom;
    float width = screenDimensions.x / camera.zoom;
    float height = screenDimensions.y / camera.zoom;
    return { left, top, width, height };
}


void GraphManager::drawNode(const Node& node,float zoom) {

    m_NodeRenderer.drawNode(node, zoom, m_Genres);
}

void GraphManager::updateGenrePosition(int nodeId, Vector2 newPos) {
    // Update position inside m_Genres for the genre that matches nodeId
    for (auto& [genre, info] : m_Genres) {
        if (info.nodeId == nodeId) {
            info.position = newPos;
            break;
        }
    }
   
}




