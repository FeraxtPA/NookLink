#pragma once
#include <raylib.h>
#include "bookManager.h"
#include "textRenderer.h"
#include "connectionManager.h"
#include "colors.h"
#include "textRenderer.h" 

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

    void drawEdges(const ConnectionManager& cm, const std::vector<Node>& nodes, const Rectangle& viewRect, float zoom);

private:
    const BookManager& m_BookManager;
    TextRenderer* m_TextRenderer; 

    Vector2 getNodePosition(int id, const std::vector<Node>& nodes) const;
    Color getStatusColor(Status status) const;
};