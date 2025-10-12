#pragma once
#include "book.h"
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <unordered_map>
class BookManager
{

public:
	 
	BookManager() = default;


	int addBook(const Book& book);

	void removeBook(int id);

	void removeBook(const Book& book);

	const std::vector<Book>& getBooks() const { return m_Books; }
		

	const Book* findBookById(int id) const;

private:
	std::vector<Book> m_Books;

	int m_NextId = 1;

};