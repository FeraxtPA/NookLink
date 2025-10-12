#pragma once
#include "book.h"
#include "ConnectionManager.h"
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
    GraphManager(const BookManager& bm, ConnectionManager& cm, Vector2 canvasSize)
        : m_BookManager(bm), m_ConnectionManager(cm), m_NodeRenderer(bm), m_CanvasSize(canvasSize)  {}

    void initializePositions() {

        m_ConnectionManager.updateConnections(m_BookManager.getBooks());
        
        resetNodeState();

        placeGenreNodes(m_ConnectionManager.getExistingGenres(), m_ConnectionManager.getGenreIdMap());
        

       
        placeBookNodes();

        for (auto& node : m_Nodes) {
            m_NodeIdMap[node.id] = &node;
        }
        
    }
   
    void updateConnections()
    {
        m_ConnectionManager.updateConnections(m_BookManager.getBooks());
    }

    void removeNodeById(int id);

  
    Rectangle getCameraViewRect(const Camera2D& camera, int screenWidth, int screenHeight);


    std::optional<Genre> getGenreByNodeId(int nodeId) const;
  


    const std::vector<Edge>& getEdges() const {
		return m_ConnectionManager.getEdges();
	}


    Node* getNodeById(int id) {
        auto it = m_NodeIdMap.find(id);
        return (it != m_NodeIdMap.end()) ? it->second : nullptr;
    }

    bool isNodeVisible(const Node& node, const Rectangle& viewRect);

    void updateGenrePosition(int nodeId, Vector2 newPos);

    void drawNode(const Node& nodes, float zoom);
    void drawEdges(float zoom, const Rectangle& viewRect);


    Font getFont()  { return m_NodeRenderer.getFont(); }

    std::vector<Node>& getNodes() { return m_Nodes; }
    Node* getDraggedNode() { return draggedNode; }
    void setDraggedNode(Node* node) { draggedNode = node; }
    void resolveNodeOverlaps(float padding = 10.0f);
 
    Node* getNodeAtPosition(Vector2 mousePos);
    const std::string getGenreNameByNodeId(int nodeId) const;

private:
    const BookManager& m_BookManager;
    ConnectionManager& m_ConnectionManager;
    std::vector<Node> m_Nodes;
    NodeRenderer m_NodeRenderer;
    GraphLayout m_GraphLayout;
    
    std::unordered_map<Genre, GenreInfo> m_Genres;

   
    std::unordered_map<int, Node*> m_NodeIdMap;

    int m_GenreIdBase = -1;
    Node* draggedNode = nullptr;
    Vector2 m_CanvasSize;




    void resetNodeState();

    float calculateCircleRadius(int nodeCount, float minRadius) const;

    void placeGenreNodes(const std::unordered_set<Genre>& genres, const std::unordered_map<Genre, int>& genreIdMap);

    void placeBookNodes();

};
