
// Manages the collection of books, including storage, search, filtering, and I/O.
// Handles book addition, deletion, modification, and JSON serialization/deserialization.
// Maintains node positions for graph visualization and provides query/sorting operations.


#pragma once
#include "book.h"
#include <vector>

#include <unordered_map>
#include <string>    
#include <filesystem>


struct NodePosition {
    float x;
    float y;
    bool locked = false;
};

enum class BookSortMode {
    IdAsc,
    AuthorAsc,
    RatingDesc,
    DateAddedDesc,
    PageCountDesc
};

class BookManager
{
public:
    BookManager() = default;

    int addBook(const Book& book);
    bool restoreBook(const Book& book);
    void removeBook(int id);
    void removeBook(const Book& book);

    const std::vector<Book>& getBooks() const { return m_Books; }

    Book* getBookById(int id);
    const Book* findBookById(int id) const;

    const Book& getRandomBookToBeRead();
    const std::vector<Book>& getBooksToBeRead();
    void sortBooks(BookSortMode mode);

    bool saveBooksToFile(const std::string& filename, const std::unordered_map<int, NodePosition>& positions = {}) const;

    bool loadBooksFromFile(const std::string& filename, std::unordered_map<int, NodePosition>& loadedPositions);

    const std::string& getLastError() const { return m_LastError; }
    bool wasRecoveredFromBackup() const { return !m_RecoveredPath.empty(); }

private:
    void setLastError(const std::string& message) const { m_LastError = message; }
    void rebuildIndex();

    std::vector<Book> m_Books;
    std::unordered_map<int, size_t> m_BookIndex;
    std::vector<Book> toBeReadBooks;
    int m_NextId = 1;
    mutable std::string m_LastError{};
    mutable std::filesystem::path m_RecoveredPath;
};
