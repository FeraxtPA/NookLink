#include "connectionManager.h"
#include <algorithm>

void ConnectionManager::updateConnections(const std::vector<Book>& books)
{
    Clear();

   

    // 1. Assign unique NEGATIVE IDs to all genres found
    for (const auto& book : books) {
        for (const auto& genre : book.getGenres()) {
            if (m_ExistingGenres.find(genre) == m_ExistingGenres.end()) {
                m_ExistingGenres.insert(genre);

               
                m_GenreIdMap[genre] = m_baseGenreId--;
            }
        }
    }

    // 2. Create Edges
    for (const auto& book : books) {
        for (const auto& genre : book.getGenres()) {
            // Retrieve the negative ID
            if (m_GenreIdMap.find(genre) != m_GenreIdMap.end()) {
                int genreId = m_GenreIdMap[genre];
                m_Edges.push_back({ book.getId(), genreId, EdgeType::BookToGenre });
            }
        }
    }
}


void ConnectionManager::removeEdgesConnectedToNode(int nodeId)
{
    // Remove edges connected to the specified node so that there are no dangling edges after a node is removed
    m_Edges.erase(
        std::remove_if(m_Edges.begin(), m_Edges.end(),
            [nodeId](const Edge& edge) {
                return edge.fromId == nodeId || edge.toId == nodeId;
            }),
        m_Edges.end()
    );
}


