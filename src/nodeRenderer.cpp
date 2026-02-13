#include "nodeRenderer.h"
#include "rlgl.h" 
#include "math.h"
#include <optional>
#include <iostream>


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

        // 1. Vykreslíme samotné pozadí uzlu (plný kruh)
        DrawCircleV(node.position, node.radius, nodeColor);

        // ==========================================
        // 2. NOVÉ: Vizuální okraje podle Statusu
        // ==========================================
        float borderThickness = 0.0f;
        Color borderColor = BLANK;

        if (book->getStatus() == Status::Reading) {
            // Pulzující efekt (funkce sin vytváøí plynulou vlnu)
            float pulse = (sin(GetTime() * 2.0f) + 1.0f) / 2.0f; // pulse je vždy od 0.0 do 1.0
            borderThickness = 3.0f + (pulse * 5.0f); // Tlouška neustále "dýchá" mezi 3 a 8

            // Svìtle krémová barva, která jemnì pulzuje i svou prùhledností
            borderColor = Fade(NookCol::TEXT_DEFAULT, alpha * (0.3f + pulse * 0.7f));
        }
        else if (book->getStatus() == Status::Read) {
            // Výrazný zlatavý okraj pro pøeètené knihy
            borderThickness = 4.0f;
            borderColor = Fade(NookCol::POPUP_BORDER, alpha);
        }
        else { // Status::ToRead
            // Jemný, šedý okraj pro knihy èekající na pøeètení
            borderThickness = 2.0f;
            borderColor = Fade(NookCol::EDGE, alpha);
        }

        // Vykreslíme samotný prstenec (okraj) kolem knihy
        DrawRing(node.position, node.radius, node.radius + borderThickness, 0, 360, 36, borderColor);
        // ==========================================


        // 3. Vykreslení textu knihy
        if (zoom >= 0.4f && m_TextRenderer) {
            float dynamicFontSize = BASE_FONT_SIZE / zoom;
            if (dynamicFontSize < 1.0f) dynamicFontSize = 1.0f;

            m_TextRenderer->DrawTextFitted(
                book->getTitle(),
                node.position,
                node.radius * 1.8f, // Max Width = 90%
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

