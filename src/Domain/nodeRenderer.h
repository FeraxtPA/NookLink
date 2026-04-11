
// Handles rendering of book and genre nodes in the graph.
// Manages visual representations, colors, labels, and node styling.


#pragma once
#include <raylib.h>
#include <unordered_map>
#include "bookManager.h"
#include "textRenderer.h"
#include "connectionManager.h"


enum class NodeType {
    Book,
    Genre
};

struct Node {
    int id;
    NodeType type;
    Vector2 position;
    float radius = 100.0f;
    bool isDragged = false;
    bool locked = false;
    bool visible = true;
};

struct GenreInfo {
    int nodeId;
    Vector2 position;
};


class NodeRenderer {
public:
   
    NodeRenderer(const BookManager& bm, TextRenderer* tr)
        : m_BookManager(bm), m_TextRenderer(tr) {}

    
    void drawNode(const Node& node, float zoom,
        const std::unordered_map<std::string, GenreInfo>& genres,
        bool isDimmed = false); 

    void drawEdges(
        const ConnectionManager& cm,
        const std::vector<Node>& nodes,
        const Rectangle& viewRect,
        float zoom,
        const std::unordered_map<int, bool>* dimmedNodes = nullptr,
        const std::unordered_map<int, size_t>* nodeIndexById = nullptr);

private:
    const BookManager& m_BookManager;
    TextRenderer* m_TextRenderer; 

    Color getStatusColor(Status status) const;
};