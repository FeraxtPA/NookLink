
// Manages connections (edges) between books and genres in the graph.
// Maintains genre information and edge relationships for graph visualization.


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

    const std::unordered_set<std::string>& getExistingGenres() const { return m_ExistingGenres; }
   
    const std::vector<Edge>& getEdges() const { return m_Edges; }

    const std::unordered_map<std::string, int>& getGenreIdMap() const { return m_GenreIdMap; }

    int getConnectedBooksCount(int genreNodeId) const;


    //Need to reset all data when loading a new library
    void Clear() {
		m_Edges.clear();
		m_ExistingGenres.clear();
		m_GenreIdMap.clear();
		m_baseGenreId = -1;
	}

private:
    std::vector<Edge> m_Edges;
    std::unordered_set<std::string> m_ExistingGenres;
    std::unordered_map<std::string, int> m_GenreIdMap;

    int m_baseGenreId = -1;

  
};
