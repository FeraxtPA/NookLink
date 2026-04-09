
// Manages graph visualization of books and their relationships.
// Handles node positioning, layout calculations, physics simulation, and grid layout.
// Provides interaction support for node selection, dragging, and graph manipulation.

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

#include "searchFilter.h"

enum class LayoutMode
{
    Physics,
    Grid
};

struct GraphConfig {
    static constexpr float BaseNodeRadius = 100.0f;
    static constexpr float GenreRadiusMultiplier = 1.5f;
    static constexpr float BookSpawnOffset = 250.0f;
};

class GraphManager {
public:
    GraphManager(const BookManager& bm, ConnectionManager& cm, Vector2 canvasSize, TextRenderer* tr);
      
    void initializePositions(bool preserveExistingPositions = true);
   
    void removeNodeById(int id);

    const Rectangle getCameraViewRect(const Camera2D& camera, Vector2 screenDimensions);

    std::optional<std::string> getGenreByNodeId(int nodeId) const;

    bool isNodeVisible(const Node& node, const Rectangle& viewRect);

    void updateGenrePosition(int nodeId, Vector2 newPos);


    void setLayoutMode(LayoutMode mode);
    LayoutMode getLayoutMode() const { return m_LayoutMode; }

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

    void setSearchQuery(const std::string& q) { m_SearchFilter.setQuery(q); }

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

    const std::vector<Node>& getNodes() const { return m_Nodes; }
    size_t getEdgeCount() const { return m_ConnectionManager.getEdges().size(); }

    Node* getNodeById(int id);

    Node* getDraggedNode() { return getNodeById(m_DraggedNodeId); }
    void setDraggedNode(int id) { m_DraggedNodeId = id; }

    void updatePhysics(float dt);

    void setLayoutDensityScale(float scale) {
        m_GraphLayout.setLayoutDensityScale(scale);
        m_IsPhysicsActive = true;
        m_GraphLayout.wakeUp();
    }
    float getLayoutDensityScale() const { return m_GraphLayout.getLayoutDensityScale(); }
    
    void wakeUpPhysics() { m_IsPhysicsActive = true; m_GraphLayout.wakeUp(); }

    Node* getNodeAtPosition(Vector2 mousePos);

    std::string getGenreNameByNodeId(int nodeId) const;

    void clearGenresAndConnections() { m_ConnectionManager.Clear(); }

    void updateConnections() { m_ConnectionManager.updateConnections(m_BookManager.getBooks()); }

    bool TryGrabNodeAt(Vector2 mousePos, bool isShiftPressed);

    void releaseDraggedNode();

    bool updateDraggedNodePosition(Vector2 mousePos);

    bool tryUnlockNodeAt(Vector2 mousePos);

    std::unordered_map<int, NodePosition> exportPositions() const;
    void applyLoadedPositions(const std::unordered_map<int, NodePosition>& loadedPos);
private:
    const BookManager& m_BookManager;
    ConnectionManager& m_ConnectionManager;
    std::vector<Node> m_Nodes;
    NodeRenderer m_NodeRenderer;
    GraphLayout m_GraphLayout;
    
    std::unordered_map<std::string, GenreInfo> m_Genres;
    

    int m_GenreIdBase = -1;
    int m_DraggedNodeId = -1; 

    Vector2 m_CanvasSize;

    std::set<Status> m_HiddenStatuses; 
    std::set<std::string> m_HiddenGenres;

    SearchFilter m_SearchFilter;

    bool m_IsPhysicsActive = true;
    LayoutMode m_LayoutMode = LayoutMode::Physics;

    std::unordered_map<int, Vector2> m_PreGridPositions;

    void resetNodeState();

    float calculateCircleRadius(int nodeCount, float minRadius) const;

    void placeGenreNodes(const std::unordered_set<std::string>& genres,
        const std::unordered_map<std::string, int>& genreIdMap,
        const std::unordered_map<std::string, Vector2>& oldPositions = {});

    void placeBookNodes(const std::unordered_map<int, Vector2>& oldPositions = {});

};
