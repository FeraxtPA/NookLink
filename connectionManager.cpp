#include "connectionManager.h"
#include <algorithm>

void ConnectionManager::updateConnections(const std::vector<Book>& books)
{
    m_Edges.clear();
    
    AssignGenreIds(books);


    AddBookToGenreEdges(books);

    //AddBookToBookEdges(books);
 

}

void ConnectionManager::AddBookToBookEdges(const std::vector<Book>& books)
{
    for (size_t i = 0; i < books.size(); ++i) {
        for (size_t j = i + 1; j < books.size(); ++j) {
            const auto& genresA = books[i].getGenres();
            const auto& genresB = books[j].getGenres();

            // Check for shared genres
            for (Genre genre : genresA) {
                if (std::find(genresB.begin(), genresB.end(), genre) != genresB.end()) {
                    // They share a genre, create a connection
                    m_Edges.push_back({ books[i].getId(), books[j].getId(), EdgeType::BookToBook });
                    break; // Only need one shared genre
                }
            }
        }
    }
}


void ConnectionManager::AssignGenreIds(const std::vector<Book>& books)
{
    //m_GenreIdMap.clear();
    //m_ExistingGenres.clear();
    int genreIdBase = m_baseGenreId;

    // First, assign unique IDs to genres
    for (const auto& book : books) {
        for (Genre genre : book.getGenres()) {
            if (m_GenreIdMap.find(genre) == m_GenreIdMap.end()) {
                m_GenreIdMap[genre] = genreIdBase--;
            }
        }
    }

    m_baseGenreId = genreIdBase; 
}

void ConnectionManager::AddBookToGenreEdges(const std::vector<Book>& books)
{
    for (const auto& book : books) {
        for (Genre genre : book.getGenres()) {
            int genreId = m_GenreIdMap[genre];
            m_Edges.push_back({ book.getId(), genreId, EdgeType::BookToGenre });
        }
    }
}

void ConnectionManager::removeEdgesConnectedToNode(int nodeId)
{
    m_Edges.erase(
        std::remove_if(m_Edges.begin(), m_Edges.end(),
            [nodeId](const Edge& edge) {
                return edge.fromId == nodeId || edge.toId == nodeId;
            }),
        m_Edges.end()
    );
}

const std::unordered_set<Genre>& ConnectionManager::getExistingGenres() 
{
  
    for(auto& [genre, id] : m_GenreIdMap) {
        m_ExistingGenres.insert(genre);
	}
    return m_ExistingGenres;
    
}
