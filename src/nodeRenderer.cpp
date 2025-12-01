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
                m_TextRenderer.drawTextCentered(book->getTitle(), node.position, node.radius, NookCol::TEXT_ONNODE);
        
    }
    else if (node.type == NodeType::Genre) {
        auto genreOpt = findGenreByNodeId(genres, node.id);
        if (!genreOpt) return; // genre not found for this node id

        Genre genre = *genreOpt;
        DrawCircleV(node.position, node.radius, NookCol::GENRE);

        if (zoom >= 0.4f)
            m_TextRenderer.drawTextCentered(genreToString(genre), node.position, node.radius, NookCol::TEXT_ONNODE);
    }
}


const float NODE_VISIBILITY_MARGIN = 100.0f;



void NodeRenderer::drawEdges(const ConnectionManager& cm, const std::vector<Node>& nodes, const Rectangle& viewRect, float zoom)
{
    // Helper function to check if a world coordinate point (with radius) is inside the view rectangle
    auto isNodeVisible = [&](const Vector2& pos, const Rectangle& rect, float radius) -> bool {
        
        // Node's bounding box coordinates
        float nodeMinX = pos.x - radius;
        float nodeMaxX = pos.x + radius;
        float nodeMinY = pos.y - radius;
        float nodeMaxY = pos.y + radius;

        // ViewRect coordinates
        float rectMinX = rect.x;
        float rectMaxX = rect.x + rect.width;
        float rectMinY = rect.y;
        float rectMaxY = rect.y + rect.height;

        // The node is visible if its bounding box overlaps the viewRect
        bool overlapsX = (nodeMaxX >= rectMinX) && (nodeMinX <= rectMaxX);
        bool overlapsY = (nodeMaxY >= rectMinY) && (nodeMinY <= rectMaxY);

        return overlapsX && overlapsY;
        };

    // Helper function to get node by ID
    auto getNodeById = [&](int id) -> const Node* {
		for (const auto& node : nodes) {
			if (node.id == id) return &node;
		}
		return nullptr;
	};

    const auto& edges = cm.getEdges();

    

    rlPushMatrix();
    rlSetLineWidth(4.0f);

    rlBegin(RL_LINES);

    for (const auto& edge : edges) {
        if (edge.type != EdgeType::BookToGenre) continue; 

        Vector2 from = getNodePosition(edge.fromId, nodes);
        Vector2 to = getNodePosition(edge.toId, nodes);

        const Node* fromNode = getNodeById(edge.fromId);
        const Node* toNode = getNodeById(edge.toId);

        float fromRadius = fromNode->radius; 
        float toRadius = toNode->radius;

        if (!fromNode || !toNode) continue;


        if (!isNodeVisible(from, viewRect, fromRadius) ||
            !isNodeVisible(to, viewRect, toRadius))
        {
            continue;
        }

        // If both nodes are visible, draw the edge
        rlColor4ub(NookCol::EDGE.r, NookCol::EDGE.g, NookCol::EDGE.b, 255);
        rlVertex2f(from.x, from.y);
        rlVertex2f(to.x, to.y);
    }

    rlEnd();

    rlPopMatrix();
}


Node* getNodeById(int id, std::vector<Node>& nodes)
{
	for (auto& node : nodes) {
		if (node.id == id) return &node;
	}
	return nullptr;
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

