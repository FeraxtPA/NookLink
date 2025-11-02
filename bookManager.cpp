#include "bookManager.h"
#include <algorithm>
#include "include/nlohmann/json.hpp" // <-- Add this

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

const Book* BookManager::findBookById(int id) const
{

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
	// Use nlohmann::json
	nlohmann::json j;

	// This will automatically use the to_json functions we wrote
	// for std::vector, Book, Status, and Genre.
	j["books"] = m_Books;

	// Also save the next ID so we don't have ID collisions after loading
	j["next_id"] = m_NextId;

	// Open an output file stream
	std::ofstream o(filename);
	if (!o.is_open()) {
		std::cerr << "Error: Could not open file for writing: " << filename << std::endl;
		return;
	}

	// Write the pretty-printed JSON to the file
	// The '4' indicates an indentation of 4 spaces
	o << j.dump(4) << std::endl;
	std::cout << "Books successfully saved to " << filename << std::endl;
}

void BookManager::loadBooksFromFile(const std::string& filename)
{
	// Open an input file stream
	std::ifstream i(filename);
	if (!i.is_open()) {
		std::cerr << "Error: Could not open file for reading: " << filename << std::endl;
		return;
	}

	nlohmann::json j;

	try {
		// Parse the file
		j = nlohmann::json::parse(i);
	}
	catch (nlohmann::json::parse_error& e) {
		std::cerr << "Error: Failed to parse JSON file: " << e.what() << std::endl;
		return;
	}

	try {
		// Vyèistíme aktuální seznam knih
		m_Books.clear();

		// --- TOTO JE NOVÝ KÓD (nahrazuje øádek 50) ---
		// Místo `get_to` projdeme pole "books" ruènì
		if (j.contains("books") && j.at("books").is_array())
		{
			// Pro každou položku v JSON poli "books"...
			for (const auto& book_json : j.at("books"))
			{
				// ...pøeveï ji na objekt Book (zavolá se from_json pro Book)
				// a vlož ji na konec našeho vektoru m_Books.
				m_Books.push_back(book_json.get<Book>());
			}
		}
		// --- KONEC NOVÉHO KÓDU ---

		// Naèteme next_id, abychom pøedešli kolizím
		m_NextId = j.at("next_id").get<int>();

		std::cout << "Books successfully loaded from " << filename << std::endl;
	}
	catch (nlohmann::json::exception& e) {
		std::cerr << "Error: JSON data is malformed: " << e.what() << std::endl;
		m_Books.clear();
		m_NextId = 1;
	}
}

