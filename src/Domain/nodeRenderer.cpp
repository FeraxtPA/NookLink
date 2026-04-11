
// Implementation of the NodeRenderer class.
// Renders individual nodes with zoom-dependent styling and text rendering.


#include "nodeRenderer.h"
#include "rlgl.h" 
#include "math.h"
#include <optional>
#include <unordered_map>
#include "colors.h"

static std::optional<std::string> findGenreByNodeId(const std::unordered_map<std::string, GenreInfo>& genres, int nodeId) {
    for (const auto& [name, info] : genres) {
        if (info.nodeId == nodeId) return name;
    }
    return std::nullopt;
}

void NodeRenderer::drawNode(const Node& node, float zoom,
    const std::unordered_map<std::string, GenreInfo>& genres,
    bool isDimmed)
{
    const float BASE_FONT_SIZE = 20.0f;

    
    float alpha = isDimmed ? 0.1f : 1.0f;

    if (node.type == NodeType::Book) {
        const Book* book = m_BookManager.findBookById(node.id);
        if (!book) return;

        // Get original color and apply transparency
        Color baseColor = getStatusColor(book->getStatus());
        Color nodeColor = Fade(baseColor, alpha);
        Color textColor = Fade(NookCol::TEXT_ONNODE, alpha);

        DrawCircleV(node.position, node.radius, nodeColor);

       
        float borderThickness = 0.0f;
        Color borderColor = BLANK;

        if (book->getStatus() == Status::Reading) {
            float pulse = static_cast<float>((sin(GetTime() * 2.0f) + 1.0f) / 2.0f); 
            borderThickness = 3.0f + (pulse * 5.0f);

         
            borderColor = Fade(NookCol::TEXT_DEFAULT, alpha * (0.3f + pulse * 0.7f));
        }
        else if (book->getStatus() == Status::Read) {
         
            borderThickness = 4.0f;
            borderColor = Fade(NookCol::POPUP_BORDER, alpha);
        }
        else {
            borderThickness = 2.0f;
            borderColor = Fade(NookCol::EDGE, alpha);
        }

       
        DrawRing(node.position, node.radius, node.radius + borderThickness, 0, 360, 36, borderColor);
     
        if (zoom >= 0.4f && m_TextRenderer) {
            float dynamicFontSize = BASE_FONT_SIZE / zoom;
            if (dynamicFontSize < 1.0f) dynamicFontSize = 1.0f;

            m_TextRenderer->DrawTextFitted(
                book->getTitle(),
                node.position,
                node.radius * 1.8f, 
                dynamicFontSize,
                textColor
            );
        }
    }
    else if (node.type == NodeType::Genre) {
        auto genreNameOpt = findGenreByNodeId(genres, node.id);
        if (!genreNameOpt) return;

        std::string genreName = *genreNameOpt;

       
        Color genreColor = Fade(NookCol::GENRE, alpha);
        Color textColor = Fade(NookCol::TEXT_ONNODE, alpha);

        DrawCircleV(node.position, node.radius, genreColor);

        if (zoom >= 0.1f && m_TextRenderer) {

            float dynamicFontSize = BASE_FONT_SIZE / zoom;
            if (dynamicFontSize < 1.0f) dynamicFontSize = 1.0f;

            m_TextRenderer->DrawTextFitted(
                genreName,
                node.position,
                node.radius * 1.8f,
                dynamicFontSize,
                textColor 
            );
        }
    }
}


const float NODE_VISIBILITY_MARGIN = 100.0f;





void NodeRenderer::drawEdges(
    const ConnectionManager& cm,
    const std::vector<Node>& nodes,
    const Rectangle& viewRect,
    float zoom,
    const std::unordered_map<int, bool>* dimmedNodes,
    const std::unordered_map<int, size_t>* nodeIndexById)
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
    std::unordered_map<int, size_t> localNodeIndexById;
    if (!nodeIndexById) {
        localNodeIndexById.reserve(nodes.size());
        for (size_t i = 0; i < nodes.size(); ++i) {
            localNodeIndexById[nodes[i].id] = i;
        }
        nodeIndexById = &localNodeIndexById;
    }

    const auto& edges = cm.getEdges();

    

    rlPushMatrix();
    rlSetLineWidth(4.0f);

    rlBegin(RL_LINES);

    for (const auto& edge : edges) {
        if (edge.type != EdgeType::BookToGenre) continue; 

        auto fromIt = nodeIndexById->find(edge.fromId);
        auto toIt = nodeIndexById->find(edge.toId);
        if (fromIt == nodeIndexById->end() || toIt == nodeIndexById->end()) continue;

        if (fromIt->second >= nodes.size() || toIt->second >= nodes.size()) continue;

        const Node* fromNode = &nodes[fromIt->second];
        const Node* toNode = &nodes[toIt->second];
        const Vector2 from = fromNode->position;
        const Vector2 to = toNode->position;
        const float fromRadius = fromNode->radius;
        const float toRadius = toNode->radius;

        

		// Hides edges if either of the connected nodes is outside the view rectangle (with margin)
        /*
        if (!isNodeVisible(from, viewRect, fromRadius) ||
            !isNodeVisible(to, viewRect, toRadius))
        {
            continue;
        }
        */

        unsigned char edgeAlpha = 255;
        if (dimmedNodes) {
            const auto fromDimIt = dimmedNodes->find(fromNode->id);
            const auto toDimIt = dimmedNodes->find(toNode->id);
            const bool fromDimmed = (fromDimIt != dimmedNodes->end()) ? fromDimIt->second : false;
            const bool toDimmed = (toDimIt != dimmedNodes->end()) ? toDimIt->second : false;
            if (fromDimmed || toDimmed) {
                edgeAlpha = 36;
            }
        }

        // If both nodes are visible, draw the edge
        rlColor4ub(NookCol::EDGE.r, NookCol::EDGE.g, NookCol::EDGE.b, edgeAlpha);
        rlVertex2f(from.x, from.y);
        rlVertex2f(to.x, to.y);
    }

    rlEnd();

    rlPopMatrix();
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

