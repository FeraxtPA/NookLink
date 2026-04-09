
// Implementation of the ConnectionManager class.
// Manages edge creation and genre identification in the book graph.


#include "connectionManager.h"
#include <algorithm>

void ConnectionManager::updateConnections(const std::vector<Book>& books)
{
    Clear();

   

    // Negative IDs for genres
    for (const auto& book : books) {
        for (const auto& genre : book.getGenres()) {
            if (m_ExistingGenres.find(genre) == m_ExistingGenres.end()) {
                m_ExistingGenres.insert(genre);

               
                m_GenreIdMap[genre] = m_baseGenreId--;
            }
        }
    }

    // Edges
    for (const auto& book : books) {
        for (const auto& genre : book.getGenres()) {
            
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

int ConnectionManager::getConnectedBooksCount(int genreNodeId) const {
    int counter = 0;
    for (const auto& edge : m_Edges) { 
        if (edge.type == EdgeType::BookToGenre && edge.toId == genreNodeId) {
            counter++;
        }
    }
    return counter;
}

