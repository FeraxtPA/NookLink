#include "nodeRenderer.h"
#include "rlgl.h" 
#include "math.h"
#include <optional>
#include <iostream>
static std::optional<Genre> findGenreByNodeId(const std::unordered_map<Genre, GenreInfo>& genres, int nodeId) {
    for (const auto& [genre, info] : genres) {
        if (info.nodeId == nodeId) {
            return genre;
        }
    }
    return std::nullopt;
}

void NodeRenderer::drawNode(const Node& node, float zoom, const std::unordered_map<Genre, GenreInfo>& genres)
{
    if (node.type == NodeType::Book) {
        const Book* book = m_BookManager.findBookById(node.id);
        if(!book) return; 
        Color color = getStatusColor(book->getStatus());
        DrawCircleV(node.position, node.radius, color);

        
        if (zoom >= 0.4f)
            m_TextRenderer.drawTextCentered(book->getTitle(), node.position, node.radius);
    }
    else if (node.type == NodeType::Genre) {
        auto genreOpt = findGenreByNodeId(genres, node.id);
        if (!genreOpt) return; // genre not found for this node id

        Genre genre = *genreOpt;
        DrawCircleV(node.position, node.radius, NookCol::GENRE);

        if (zoom >= 0.4f)
            m_TextRenderer.drawTextCentered(genreToString(genre), node.position, node.radius);
    }
}


void NodeRenderer::drawEdges(const ConnectionManager& cm, const std::vector<Node>& nodes, const Rectangle& viewRect)
{
    const auto& edges = cm.getEdges();
    rlPushMatrix();
    rlSetLineWidth(4.0f);
    
    rlBegin(RL_LINES);
    for (const auto& edge : edges) {
        if (edge.type != EdgeType::BookToGenre) continue; // Skip non-BookToGenre edges

        Vector2 from = getNodePosition(edge.fromId, nodes);
        Vector2 to = getNodePosition(edge.toId, nodes);



        rlColor4ub(NookCol::EDGE.r, NookCol::EDGE.g, NookCol::EDGE.b, 255);
        rlVertex2f(from.x, from.y);
        rlVertex2f(to.x, to.y);
    }

    rlEnd();
 
    rlPopMatrix();
   
}


Vector2 NodeRenderer::getNodePosition(int id, const std::vector<Node>& nodes) const
{
    for (const auto& node : nodes) {
        if (node.id == id) return node.position;
    }
    return { 0, 0 };
}

Color NodeRenderer::getStatusColor(Status status) const
{
    switch (status) {
    case Status::ToRead: return 		NookCol::TO_READ;
    case Status::Reading: return 		NookCol::CURRENTLY_READING;
    case Status::Read: return 		NookCol::READ;
    default: return  { 124, 127, 147,255 };
    }
}





/*
namespace std {
    template<>
    struct hash<Color> {
        size_t operator()(const Color& c) const noexcept {
            // Pack RGBA into a 32-bit int (or size_t)
            return (size_t(c.r) << 24) | (size_t(c.g) << 16) | (size_t(c.b) << 8) | size_t(c.a);
        }
    };
}


inline bool operator==(const Color& lhs, const Color& rhs)
{
    return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b && lhs.a == rhs.a;
}

inline bool operator!=(const Color& lhs, const Color& rhs)
{
    return !(lhs == rhs);
}


void NodeRenderer::drawNode(std::vector<Node> m_Nodes, float zoom, std::unordered_map<int, Genre>& genreIdLookup)
{
    // Maps from Color to vector of nodes that share that color
    std::unordered_map<Color, std::vector<Vector2>> nodesByColor;

    // Group nodes by color
    for (const auto& node : m_Nodes)
    {
        Color color;
        if (node.type == NodeType::Book) {
            const Book& book = m_BookManager.findBookById(node.id);
            color = getStatusColor(book.getStatus());
        }
        else if (node.type == NodeType::Genre) {
            color = VIOLET;
        }
        else {
            color = LIGHTGRAY;
        }

        nodesByColor[color].push_back(node.position);
    }

    // For each color group, batch draw circles
    for (auto& [color, positions] : nodesByColor)
    {

        rlColor4ub(color.r, color.g, color.b, color.a);

        rlBegin(RL_QUADS);

        for (const auto& pos : positions)
        {
            // Here draw a filled circle at pos.
            // Since rlgl does not provide a built-in circle primitive, you can draw a polygon approximating a circle:

            constexpr int segments = 24;

            for (int i = 0; i < segments; i++)
            {
                float angle1 = 2.0f * PI * i / segments;
                float angle2 = 2.0f * PI * (i + 1) / segments;

                // Points on the outer edge
                float x1 = pos.x + cosf(angle1) * 100.0f;
                float y1 = pos.y + sinf(angle1) * 100.0f;
                float x2 = pos.x + cosf(angle2) * 100.0f;
                float y2 = pos.y + sinf(angle2) * 100.0f;

                // We use the center point twice to create a thin quad "strip"
                rlVertex2f(pos.x, pos.y);
                rlVertex2f(pos.x, pos.y);
                rlVertex2f(x2, y2);
                rlVertex2f(x1, y1);
            }
        }
        rlEnd();
    }







    // Draw text after all circles if zoom threshold met
    if (zoom > 0.15f)
    {
        for (const auto& node : m_Nodes)
        {
            if (node.type == NodeType::Book) {
                const Book& book = m_BookManager.findBookById(node.id);
                m_TextRenderer.drawTextCentered(book.getTitle(), node.position, node.radius);
            }
            else if (node.type == NodeType::Genre) {
                const Genre& genre = genreIdLookup[node.id];
                m_TextRenderer.drawTextCentered(genreToString(genre), node.position, node.radius);
            }
        }
    }

}

*/
