#pragma once
#include "book.h"
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <string>    
#include <fstream>   
#include <iostream>   

// NOVÁ STRUKTURA pro bezpeèný pøenos souøadnic bez nutnosti Raylibu
struct NodePosition {
    float x;
    float y;
    bool locked = false;
};

class BookManager
{
public:
    BookManager() = default;

    int addBook(const Book& book);
    void removeBook(int id);
    void removeBook(const Book& book);

    const std::vector<Book>& getBooks() const { return m_Books; }

    Book* getBookById(int id);
    const Book* findBookById(int id) const;

    const Book& getRandomBookToBeRead();
    const std::vector<Book>& getBooksToBeRead();

    // ZMÌNÌNÉ FUNKCE: Pøidali jsme podporu pro mapu pozic
    void saveBooksToFile(const std::string& filename, const std::unordered_map<int, NodePosition>& positions = {}) const;

    // Návratová hodnota nám teï vrátí mapu naètených pozic (pokud nìjaké v souboru byly)
    std::unordered_map<int, NodePosition> loadBooksFromFile(const std::string& filename);

private:
    std::vector<Book> m_Books;
    std::vector<Book> toBeReadBooks;
    int m_NextId = 1;
};