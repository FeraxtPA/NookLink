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


class GraphManager {
public:
    GraphManager(const BookManager& bm, ConnectionManager& cm, Vector2 canvasSize, TextRenderer* tr);
      
    void initializePositions();
   
    void removeNodeById(int id);

    const Rectangle getCameraViewRect(const Camera2D& camera, Vector2 screenDimensions);

    const std::optional<std::string> getGenreByNodeId(int nodeId) const;

    bool isNodeVisible(const Node& node, const Rectangle& viewRect);

    void updateGenrePosition(int nodeId, Vector2 newPos);

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

    void resetNodeState();

    float calculateCircleRadius(int nodeCount, float minRadius) const;

    void placeGenreNodes(const std::unordered_set<std::string>& genres, const std::unordered_map<std::string, int>& genreIdMap);

    void placeBookNodes();

};
