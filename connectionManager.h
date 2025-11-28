#pragma once
#include "book.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>

enum class EdgeType {
    BookToBook,
    BookToGenre
};

struct Edge {
    int fromId;
    int toId;
    EdgeType type;
};

class ConnectionManager {
public:
    void updateConnections(const std::vector<Book>& books);

    void removeEdgesConnectedToNode(int nodeId);

    const std::unordered_set<Genre>& getExistingGenres();
   
    const std::vector<Edge>& getEdges() const { return m_Edges; }

    const std::unordered_map<Genre, int>& getGenreIdMap() const { return m_GenreIdMap; }


    //Need to reset all data when loading a new library(åoading a file)
    void Clear() {
		m_Edges.clear();
		m_ExistingGenres.clear();
		m_GenreIdMap.clear();
		m_baseGenreId = -1;
	}

private:
    std::vector<Edge> m_Edges;
    std::unordered_set<Genre> m_ExistingGenres;
    std::unordered_map<Genre, int> m_GenreIdMap;

    int m_baseGenreId = -1;

    void AddBookToBookEdges(const std::vector<Book>& books);
    void AssignGenreIds(const std::vector<Book>& books);
    void AddBookToGenreEdges(const std::vector<Book>& books);
};
