#include "bookManager.h"
#include <algorithm>
#include <string>     
#include <fstream>    
#include <iostream>  
#include "../include/nlohmann/json.hpp" 
#include <random>

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

const std::vector<Book>& BookManager::getBooksToBeRead()
{	
	toBeReadBooks.clear();
	for (const auto& book : m_Books) {
		if (book.getStatus() == Status::ToRead) {
			toBeReadBooks.push_back(book);
		}
	}
	return toBeReadBooks;
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

const Book& BookManager::getRandomBookToBeRead()
{
	static std::random_device rd;
	static std::mt19937 gen(rd());

	const auto& toBeReadBooks = getBooksToBeRead();

	if (toBeReadBooks.empty()) {
		throw std::runtime_error("No books available to be read.");
	}
	std::uniform_int_distribution<> dis(0, static_cast<int>(toBeReadBooks.size()) - 1);

	auto randomIndex = dis(gen);
	return toBeReadBooks[randomIndex];
}

void BookManager::saveBooksToFile(const std::string& filename, const std::unordered_map<int, NodePosition>& positions) const
{
	nlohmann::json j;

	j["books"] = m_Books;
	j["next_id"] = m_NextId;

	// NOVÉ: Uložení pozic bokem
	nlohmann::json posJson = nlohmann::json::object();
	for (const auto& [id, pos] : positions) {
		// Ukládáme jako "ID": { "x": 100, "y": 200 }
		posJson[std::to_string(id)] = { {"x", pos.x}, {"y", pos.y}, {"locked", pos.locked} };
	}
	j["positions"] = posJson;

	std::ofstream o(filename);
	if (!o.is_open()) {
		std::cerr << "Error: Could not open file for writing: " << filename << std::endl;
		return;
	}

	o << j.dump(4) << std::endl;
	std::cout << "Books and positions successfully saved to " << filename << std::endl;
}
std::unordered_map<int, NodePosition> BookManager::loadBooksFromFile(const std::string& filename)
{
    std::unordered_map<int, NodePosition> loadedPositions;
    std::ifstream i(filename);
    
    if (!i.is_open()) {
        std::cerr << "Error: Could not open file for reading: " << filename << std::endl;
        return loadedPositions;
    }

    nlohmann::json j;
    try {
        j = nlohmann::json::parse(i);
    }
    catch (nlohmann::json::parse_error& e) {
        std::cerr << "Error: Failed to parse JSON file: " << e.what() << std::endl;
        return loadedPositions;
    }

    try {
        // Pùvodní naèítání knih
        m_Books = j.value("books", std::vector<Book>());
        m_NextId = j.value("next_id", 1);

        // NOVÉ: Naètení pozic
        if (j.contains("positions")) {
            for (auto& el : j["positions"].items()) {
                int id = std::stoi(el.key());
                float x = el.value()["x"];
                float y = el.value()["y"];
				bool locked = el.value().value("locked", false);
                loadedPositions[id] = { x, y , locked};
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error reconstructing books: " << e.what() << std::endl;
    }

    return loadedPositions; // Vrátíme pozice do hlavní aplikace
}
