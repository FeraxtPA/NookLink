#pragma once
#include "book.h"
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <unordered_map>

#include <string>    
#include <fstream>   
#include <iostream>   

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


	void saveBooksToFile(const std::string& filename) const;
	void loadBooksFromFile(const std::string& filename);

private:
	std::vector<Book> m_Books;

	int m_NextId = 1;

};