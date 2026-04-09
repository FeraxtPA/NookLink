#pragma once
#include "book.h"
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <string>    
#include <fstream>   

// NOV� STRUKTURA pro bezpe�n� p�enos sou�adnic bez nutnosti Raylibu
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

    // ZM�N�N� FUNKCE: P�idali jsme podporu pro mapu pozic
    bool saveBooksToFile(const std::string& filename, const std::unordered_map<int, NodePosition>& positions = {}) const;

    // Na��t� knihy a pozice; vrac� false p�i chyb� a detail je v getLastError().
    bool loadBooksFromFile(const std::string& filename, std::unordered_map<int, NodePosition>& loadedPositions);

    const std::string& getLastError() const { return m_LastError; }

private:
    void setLastError(const std::string& message) const { m_LastError = message; }

    std::vector<Book> m_Books;
    std::vector<Book> toBeReadBooks;
    int m_NextId = 1;
    mutable std::string m_LastError{};
};