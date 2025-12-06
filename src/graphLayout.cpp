#include "graphLayout.h"

// Resolve overlaps between nodes by adjusting their positions
void GraphLayout::resolveNodeOverlaps(float padding, std::vector<Node>& m_Nodes)
{
    float maxDisplacement = 20.0f;

    for (size_t i = 0; i < m_Nodes.size(); ++i) {
        for (size_t j = i + 1; j < m_Nodes.size(); ++j) {
            Node& a = m_Nodes[i];
            Node& b = m_Nodes[j];

            float totalRadius = a.radius + b.radius + padding;
            Vector2 delta = Vector2Subtract(b.position, a.position);
            float dist = Vector2Length(delta);

            if (dist < totalRadius && dist > 0.001f) {
                Vector2 direction = Vector2Normalize(delta);
                float overlap = totalRadius - dist;
                Vector2 displacement = Vector2Scale(direction, std::min(overlap * 0.5f, maxDisplacement));

                
                bool aUserFixed = a.locked || a.isDragged;
                bool bUserFixed = b.locked || b.isDragged;

                bool aIsGenre = (a.type == NodeType::Genre);
                bool bIsGenre = (b.type == NodeType::Genre);

                float aMoveFactor = 0.0f;
                float bMoveFactor = 0.0f;

               

                // Both fixed none moves
                if (aUserFixed && bUserFixed) {
                    aMoveFactor = 0.0f;
                    bMoveFactor = 0.0f;
                }
                // 2. If A is fixed (Dragged/Locked)...
                else if (aUserFixed) {
                    aMoveFactor = 0.0f; // A never yields

                   
                    if (aIsGenre) {
                        // Dragged GENRE pushes everything (Books AND Genres)
                        bMoveFactor = 1.0f;
                    }
                    else {
                        // Dragged BOOK pushes Books, but CANNOT push Genres
                        if (bIsGenre) bMoveFactor = 0.0f;
                        else bMoveFactor = 1.0f;
                    }
                }
                //If B is fixed (Dragged/Locked)...
                else if (bUserFixed) {
                    bMoveFactor = 0.0f; // B never yields

                    // Does A yield?
                    if (bIsGenre) {
                        // Dragged GENRE pushes everything
                        aMoveFactor = 1.0f;
                    }
                    else {
                        // Dragged BOOK pushes Books, but CANNOT push Genres
                        if (aIsGenre) aMoveFactor = 0.0f;
                        else aMoveFactor = 1.0f;
                    }
                }
                // 4. Neither is fixed (Standard Physics)
                else {
                    // Genre vs Genre
                    if (aIsGenre && bIsGenre) {
                        aMoveFactor = 0.5f;
                        bMoveFactor = 0.5f;
                    }
                    // Genre vs Book: Genre stays, Book moves
                    else if (aIsGenre) {
                        aMoveFactor = 0.0f;
                        bMoveFactor = 1.0f;
                    }
                    // Book vs Genre: Book moves, Genre stays
                    else if (bIsGenre) {
                        aMoveFactor = 1.0f;
                        bMoveFactor = 0.0f;
                    }
                    // Book vs Book
                    else {
                        aMoveFactor = 0.5f;
                        bMoveFactor = 0.5f;
                    }
                }

                
                if (aMoveFactor > 0.0f)
                    a.position = Vector2Subtract(a.position, Vector2Scale(displacement, aMoveFactor));

                if (bMoveFactor > 0.0f)
                    b.position = Vector2Add(b.position, Vector2Scale(displacement, bMoveFactor));
            }
        }
    }
}
// Apply spring constraints between book nodes and genre nodes
// Adjust spring length based on the number of books in each genre
void GraphLayout::applySpringConstraints(
    std::vector<Node>& nodes,
    const std::unordered_map<int, std::vector<int>>& bookToGenreMap,
    const std::unordered_map<int, int>& genreToBookCount,
    float baseSpringLength,
    float springConstant)
{
    for (Node& bookNode : nodes) {
        if (bookNode.type != NodeType::Book) continue;

        auto it = bookToGenreMap.find(bookNode.id);
        if (it == bookToGenreMap.end()) continue;

        const std::vector<int>& connectedGenres = it->second;

        for (int genreNodeId : connectedGenres) {
            auto genreIt = std::find_if(nodes.begin(), nodes.end(),
                [genreNodeId](const Node& n) { return n.id == genreNodeId; });
            if (genreIt == nodes.end()) continue;

            const Node& genreNode = *genreIt;

            int bookCount = 1;
            auto countIt = genreToBookCount.find(genreNodeId);
            if (countIt != genreToBookCount.end()) {
                bookCount = countIt->second;
            }

            float dynamicSpringLength = baseSpringLength * std::log2(bookCount + 1);
           


            Vector2 delta = Vector2Subtract(bookNode.position, genreNode.position);
            float dist = Vector2Length(delta);
            if (dist < 0.01f) dist = 0.01f;

            if (dist > dynamicSpringLength) {
                Vector2 direction = Vector2Normalize(delta);
                float displacementAmount = (dist - dynamicSpringLength) * springConstant;

                Vector2 displacement = Vector2Scale(direction, displacementAmount);

                if (!bookNode.locked) {
                    bookNode.position = Vector2Subtract(bookNode.position, displacement);
                }
            }
        }
    }
}











