#pragma once
#include "book.h"
#include "connectionManager.h"
#include <raylib.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <cmath>
#include "raymath.h"
#include "bookManager.h"
#include <algorithm>
#include <random>
#include "nodeRenderer.h"
#include "graphLayout.h"
#include "rlgl.h"
#include <optional>
#include <set>

class GraphManager {
public:
    GraphManager(const BookManager& bm, ConnectionManager& cm, Vector2 canvasSize, TextRenderer* tr);
      
    void initializePositions();
   
    void removeNodeById(int id);

    const Rectangle getCameraViewRect(const Camera2D& camera, Vector2 screenDimensions);

    const std::optional<std::string> getGenreByNodeId(int nodeId) const;

    bool isNodeVisible(const Node& node, const Rectangle& viewRect);

    void updateGenrePosition(int nodeId, Vector2 newPos);

    std::vector<std::string> getAllGenreNames() const {
        std::vector<std::string> names;
        for (const auto& pair : m_Genres) {
            names.push_back(pair.first);
        }
        return names;
    }

    bool isStatusVisible(Status s) const {
        return m_HiddenStatuses.find(s) == m_HiddenStatuses.end();
    }
    bool isGenreVisible(const std::string& g) const {
        return m_HiddenGenres.find(g) == m_HiddenGenres.end();
    }

    void setSearchQuery(std::string q) { m_SearchQuery = q; }
    void drawNodes(float zoom, const Rectangle& viewRect);

    void toggleStatusVisibility(Status status) {
        if (m_HiddenStatuses.count(status)) m_HiddenStatuses.erase(status);
        else m_HiddenStatuses.insert(status);

        recalculateVisibility(); // Update the nodes immediately
    }

    void toggleGenreVisibility(const std::string& genre) {
      
        if (m_HiddenGenres.count(genre)) m_HiddenGenres.erase(genre);
        else m_HiddenGenres.insert(genre);

        recalculateVisibility();
    }

    void recalculateVisibility(); 

    void drawNode(const Node& node, float zoom);
    void drawEdges(float zoom,const Rectangle& viewRect);

    int getNumOfConnectedBooks(int genreNodeId) const;

    std::vector<Node>& getNodes()  { return m_Nodes; }
    Node* getDraggedNode() { return draggedNode; }
    void setDraggedNode(Node* node) { draggedNode = node; }

    void resolveNodeOverlaps(float padding = 10.0f);
 
    Node* getNodeAtPosition(Vector2 mousePos);
    const std::string getGenreNameByNodeId(int nodeId) const; 

    void clearGenresAndConnections() { m_ConnectionManager.Clear(); }

    void updateConnections() { m_ConnectionManager.updateConnections(m_BookManager.getBooks()); }
private:
    const BookManager& m_BookManager;
    ConnectionManager& m_ConnectionManager;
    std::vector<Node> m_Nodes;
    NodeRenderer m_NodeRenderer;
    GraphLayout m_GraphLayout;
    
    std::unordered_map<std::string, GenreInfo> m_Genres;
    std::unordered_map<int, Node*> m_NodeIdMap;

    int m_GenreIdBase = -1;
    Node* draggedNode = nullptr;
    Vector2 m_CanvasSize;

    std::set<Status> m_HiddenStatuses; 
    std::set<std::string> m_HiddenGenres;

    std::string m_SearchQuery; 

    void resetNodeState();

    float calculateCircleRadius(int nodeCount, float minRadius) const;

    void placeGenreNodes(const std::unordered_set<std::string>& genres,
        const std::unordered_map<std::string, int>& genreIdMap,
        const std::unordered_map<std::string, Vector2>& oldPositions = {});

    void placeBookNodes(const std::unordered_map<int, Vector2>& oldPositions = {});

};
