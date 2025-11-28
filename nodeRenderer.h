#pragma once
#include <raylib.h>
#include "bookManager.h"
#include "textRenderer.h"
#include "connectionManager.h"
#include "colors.h"

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
};

struct GenreInfo {
    int nodeId;
    Vector2 position;
};


class NodeRenderer {
public:
    NodeRenderer(const BookManager& bm)
        : m_BookManager(bm) {}

    void drawNode(const Node& node, float zoom, const std::unordered_map<Genre, GenreInfo>& genres);
    void drawEdges(const ConnectionManager& cm, const std::vector<Node>& nodes, const Rectangle& viewRect, float zoom);


private:
    const BookManager& m_BookManager;
    TextRenderer m_TextRenderer;

    Vector2 getNodePosition(int id, const std::vector<Node>& nodes) const;
    Color getStatusColor(Status status) const;
};
