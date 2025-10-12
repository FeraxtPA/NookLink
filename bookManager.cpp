#include "bookManager.h"
#include <algorithm>
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

