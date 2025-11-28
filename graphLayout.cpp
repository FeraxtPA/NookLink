#include "graphLayout.h"

// Resolve overlaps between nodes by adjusting their positions
void GraphLayout::resolveNodeOverlaps(float padding, std::vector<Node>& m_Nodes)
{
    // Maximum displacement to avoid excessive movement
    float maxDisplacement = 100.0f;

    for (size_t i = 0; i < m_Nodes.size(); ++i) {
        for (size_t j = i + 1; j < m_Nodes.size(); ++j) {
            Node& a = m_Nodes[i];
            Node& b = m_Nodes[j];

            float totalRadius = a.radius + b.radius + padding;
            Vector2 delta = Vector2Subtract(b.position, a.position);
            float dist = Vector2Length(delta);

            // Check for overlap
            if (dist < totalRadius && dist > 0.001f) {

                
                Vector2 direction = Vector2Normalize(delta);
                float overlap = totalRadius - dist;
                Vector2 displacement = Vector2Scale(direction, std::min(overlap * 0.5f, maxDisplacement));

               
                if (a.locked && !b.locked) {
                    b.position = Vector2Add(b.position, displacement);
                }
                else if (!a.locked && b.locked) {
                    a.position = Vector2Subtract(a.position, displacement);
                }
                else {

                    // Apply different weights based on node type
                    // Can't move genres nodes by other genre nodes
                    float aWeight = (a.type == NodeType::Genre) ? 0.0f : 1.0f;
                    float bWeight = (b.type == NodeType::Genre) ? 0.0f : 1.0f;

                    a.position = Vector2Subtract(a.position, Vector2Scale(displacement, aWeight));
                    b.position = Vector2Add(b.position, Vector2Scale(displacement, bWeight));
                }
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











