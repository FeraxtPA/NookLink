
// Manages the collection of books, including storage, search, filtering, and I/O.
// Handles book addition, deletion, modification, and JSON serialization/deserialization.
// Maintains node positions for graph visualization and provides query/sorting operations.


#pragma once
#include "book.h"
#include <vector>

#include <unordered_map>
#include <string>    


struct NodePosition {
    float x;
    float y;
    bool locked = false;
};

enum class BookSortMode {
    IdAsc,
    AuthorAsc,
    RatingDesc,
    DateAddedDesc
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

private:
    void setLastError(const std::string& message) const { m_LastError = message; }

    std::vector<Book> m_Books;
    std::vector<Book> toBeReadBooks;
    int m_NextId = 1;
    mutable std::string m_LastError{};
};