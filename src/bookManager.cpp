#include "bookManager.h"
#include <algorithm>
#include <string>     
#include <fstream>    
#include <iostream>  
#include "../include/nlohmann/json.hpp" 

int BookManager::addBook(const Book& book)
{
	Book newBook = book;
	newBook.setId(m_NextId++);
	m_Books.push_back(newBook);
	
	return newBook.getId();
}

void BookManager::removeBook(int id)
{
	m_Books.erase(std::remove_if(m_Books.begin(), m_Books.end(),
		[id](const Book& book) { return book.getId() == id; }), m_Books.end());
	
}

void BookManager::removeBook(const Book& book)
{
	removeBook(book.getId());
}

Book* BookManager::getBookById(int id) {
	for (auto& book : m_Books) {
		if (book.getId() == id) {
			return &book;
		}
	}
	return nullptr;
}
const Book* BookManager::findBookById(int id) const
{
	//Binary search since books are stored sorted by id
	auto it = std::lower_bound(m_Books.begin(), m_Books.end(), id,
		[](const Book& book, int value) {
			return book.getId() < value;
		});

	if (it != m_Books.end() && it->getId() == id) {
		return &(*it);
	}
	else {
		return nullptr;
	}
	
}

void BookManager::saveBooksToFile(const std::string& filename) const
{

	nlohmann::json j;


	j["books"] = m_Books;

	//Save last id for adding new books later
	j["next_id"] = m_NextId;

	
	std::ofstream o(filename);
	if (!o.is_open()) {
		std::cerr << "Error: Could not open file for writing: " << filename << std::endl;
		return;
	}


	o << j.dump(4) << std::endl;
	std::cout << "Books successfully saved to " << filename << std::endl;
}

void BookManager::loadBooksFromFile(const std::string& filename)
{
	
	std::ifstream i(filename);
	if (!i.is_open()) {
		std::cerr << "Error: Could not open file for reading: " << filename << std::endl;
		return;
	}

	nlohmann::json j;

	try {
		j = nlohmann::json::parse(i);
	}
	catch (nlohmann::json::parse_error& e) {
		std::cerr << "Error: Failed to parse JSON file: " << e.what() << std::endl;
		return;
	}

	try {
		
	    //Clear books before loading from save
		m_Books.clear();
		


		if (j.contains("books") && j.at("books").is_array())
		{
		
			for (const auto& book_json : j.at("books"))
			{
			
				m_Books.push_back(book_json.get<Book>());
			}
		}
		
		// Needed if any new book will be added, so that the id continues from where left off
		m_NextId = j.at("next_id").get<int>();

	
	}
	catch (nlohmann::json::exception& e) {
		std::cerr << "Error: JSON data is malformed: " << e.what() << std::endl;
		m_Books.clear();
		m_NextId = 1;
	}
}

