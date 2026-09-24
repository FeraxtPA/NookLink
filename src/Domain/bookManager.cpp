
// Implementation of the BookManager class.
// Handles book persistence, sorting, filtering, and graph node management.


#include "bookManager.h"
#include <algorithm>
#include <cctype>
#include <string>     
#include <fstream>    
#include <nlohmann/json.hpp>
#include <random>
#include <filesystem>
#include "logging.h"
#include "fileStorage.h"
#include <limits>
#include <unordered_set>

namespace fs = std::filesystem;

namespace {
int ParseDateDDMMYYYYToSortable(const std::string& date)
{
    // Convert DD.MM.YYYY to YYYYMMDD so plain integer comparison matches chronology.
	if (date.size() != 10 || date[2] != '.' || date[5] != '.') {
		return 0;
	}

	for (size_t i = 0; i < date.size(); ++i) {
		if (i == 2 || i == 5) {
			continue;
		}
		if (date[i] < '0' || date[i] > '9') {
			return 0;
		}
	}

	const int day = std::stoi(date.substr(0, 2));
	const int month = std::stoi(date.substr(3, 2));
	const int year = std::stoi(date.substr(6, 4));
	return (year * 10000) + (month * 100) + day;
}

std::string ToLowerCopy(std::string value)
{
	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
		return static_cast<char>(std::tolower(c));
	});
	return value;
}
}

int BookManager::addBook(const Book& book)
{
	if (m_NextId >= std::numeric_limits<int>::max()) {
		throw std::overflow_error("No more book IDs are available");
	}
	Book newBook = book;
	newBook.setId(m_NextId++);
	m_Books.push_back(newBook);
	m_BookIndex[newBook.getId()] = m_Books.size() - 1;
	
	return newBook.getId();
}

bool BookManager::restoreBook(const Book& book)
{
	if (book.getId() <= 0 || book.getId() >= std::numeric_limits<int>::max() ||
		findBookById(book.getId()) != nullptr) {
		Log::Warn("restoreBook skipped: ID already exists " + std::to_string(book.getId()));
		return false;
	}

	m_Books.push_back(book);
	m_BookIndex[book.getId()] = m_Books.size() - 1;
	if (book.getId() >= m_NextId) {
		m_NextId = book.getId() + 1;
	}

	return true;
}

void BookManager::removeBook(int id)
{
	m_Books.erase(std::remove_if(m_Books.begin(), m_Books.end(),
		[id](const Book& book) { return book.getId() == id; }), m_Books.end());
	rebuildIndex();
	
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

void BookManager::rebuildIndex()
{
	m_BookIndex.clear();
	for (size_t index = 0; index < m_Books.size(); ++index) {
		m_BookIndex.emplace(m_Books[index].getId(), index);
	}
}

Book* BookManager::getBookById(int id) {
	const auto it = m_BookIndex.find(id);
	return it == m_BookIndex.end() ? nullptr : &m_Books[it->second];
}
const Book* BookManager::findBookById(int id) const
{
	const auto it = m_BookIndex.find(id);
	return it == m_BookIndex.end() ? nullptr : &m_Books[it->second];
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

void BookManager::sortBooks(BookSortMode mode)
{
	switch (mode) {
	case BookSortMode::AuthorAsc:
		std::sort(m_Books.begin(), m_Books.end(), [](const Book& a, const Book& b) {
			const std::string aa = ToLowerCopy(a.getAuthor());
			const std::string bb = ToLowerCopy(b.getAuthor());
			if (aa == bb) {
				return a.getId() < b.getId();
			}
			return aa < bb;
		});
		break;
	case BookSortMode::RatingDesc:
		std::sort(m_Books.begin(), m_Books.end(), [](const Book& a, const Book& b) {
			if (a.getRating() == b.getRating()) {
				return a.getId() < b.getId();
			}
			return a.getRating() > b.getRating();
		});
		break;
	case BookSortMode::DateAddedDesc:
		std::sort(m_Books.begin(), m_Books.end(), [](const Book& a, const Book& b) {
			const int da = ParseDateDDMMYYYYToSortable(a.getDateAdded());
			const int db = ParseDateDDMMYYYYToSortable(b.getDateAdded());
			if (da == db) {
				return a.getId() < b.getId();
			}
			return da > db;
		});
		break;
   case BookSortMode::PageCountDesc:
		std::sort(m_Books.begin(), m_Books.end(), [](const Book& a, const Book& b) {
			if (a.getPageCount() == b.getPageCount()) {
				return a.getId() < b.getId();
			}
			return a.getPageCount() > b.getPageCount();
		});
		break;
	case BookSortMode::IdAsc:
	default:
		std::sort(m_Books.begin(), m_Books.end(), [](const Book& a, const Book& b) {
			return a.getId() < b.getId();
		});
		break;
	}
	rebuildIndex();
}

bool BookManager::saveBooksToFile(const std::string& filename, const std::unordered_map<int, NodePosition>& positions) const
{
    setLastError("");
    try {
        if (filename.empty()) throw std::runtime_error("The target filename is empty");
        nlohmann::json document;
        document["format_version"] = 1;
        document["books"] = m_Books;
        document["next_id"] = m_NextId;
        auto positionJson = nlohmann::json::object();
        for (const auto& [id, position] : positions) {
            if (!std::isfinite(position.x) || !std::isfinite(position.y)) {
                throw std::runtime_error("Cannot save a non-finite node position");
            }
            positionJson[std::to_string(id)] = {
                {"x", position.x}, {"y", position.y}, {"locked", position.locked}
            };
        }
        document["positions"] = std::move(positionJson);
        const auto target = fs::absolute(fs::path(filename)).lexically_normal();
        std::string error;
        // A recovered library must not overwrite its good backup with the damaged primary.
        if (!FileStorage::WriteAtomically(target, document.dump(4) + "\n", error,
                                         target != m_RecoveredPath)) {
            throw std::runtime_error(error);
        }
        m_RecoveredPath.clear();
        Log::Info("Books and positions successfully saved to " + filename);
        return true;
    }
    catch (const std::exception& exception) {
        setLastError("Save failed: " + std::string(exception.what()));
        Log::Error(getLastError());
        return false;
    }
}

bool BookManager::loadBooksFromFile(const std::string& filename, std::unordered_map<int, NodePosition>& loadedPositions)
{
    setLastError("");
    if (filename.empty()) {
        setLastError("Load failed: source filename is empty");
        return false;
    }

    struct Library {
        std::vector<Book> books;
        std::unordered_map<int, NodePosition> positions;
        int nextId = 1;
    };

    auto readLibrary = [&](const fs::path& path, Library& library, std::string& error) {
        try {
            std::ifstream input(path);
            if (!input) throw std::runtime_error("Could not open " + path.string());
            const auto document = nlohmann::json::parse(input);
            if (!document.is_object() || !document.contains("books") || !document["books"].is_array()) {
                throw std::runtime_error("Expected an object containing a books array");
            }
            if (document.value("format_version", 1) != 1) {
                throw std::runtime_error("Unsupported library format version");
            }

            Library candidate;
            std::unordered_set<int> ids;
            for (const auto& entry : document["books"]) {
                if (!entry.is_object() || !entry.contains("id") || !entry["id"].is_number_integer() ||
                    entry["id"] <= 0 || entry["id"] >= std::numeric_limits<int>::max()) {
                    throw std::runtime_error("A book has an invalid ID");
                }
                Book book = entry.get<Book>();
                if (!ids.insert(book.getId()).second) throw std::runtime_error("Duplicate book ID");
                if (!std::isfinite(book.getRating()) || book.getRating() < 0 || book.getRating() > 5) {
                    throw std::runtime_error("A book has an invalid rating");
                }
                candidate.nextId = std::max(candidate.nextId, book.getId() + 1);
                candidate.books.push_back(std::move(book));
            }

            if (document.contains("next_id")) {
                const auto& next = document["next_id"];
                if (!next.is_number_integer() || next < 1 || next > std::numeric_limits<int>::max()) {
                    throw std::runtime_error("Invalid next_id");
                }
                candidate.nextId = std::max(candidate.nextId, next.get<int>());
            }

            if (document.contains("positions")) {
                if (!document["positions"].is_object()) throw std::runtime_error("Expected a positions object");
                for (const auto& [key, value] : document["positions"].items()) {
                    size_t parsed = 0;
                    const int id = std::stoi(key, &parsed);
                    if (parsed != key.size()) throw std::runtime_error("Invalid node position ID");
                    NodePosition position{value.at("x").get<float>(), value.at("y").get<float>(),
                                          value.value("locked", false)};
                    if (!std::isfinite(position.x) || !std::isfinite(position.y)) {
                        throw std::runtime_error("Invalid node coordinates");
                    }
                    if (!candidate.positions.emplace(id, position).second) {
                        throw std::runtime_error("Duplicate node position ID");
                    }
                }
            }
            library = std::move(candidate);
            return true;
        }
        catch (const std::exception& exception) {
            error = exception.what();
            return false;
        }
    };

    try {
        const fs::path primary = fs::absolute(fs::path(filename)).lexically_normal();
        fs::path backup = primary;
        backup += ".bak";
        Library library;
        std::string primaryError;
        bool recovered = false;
        if (!readLibrary(primary, library, primaryError)) {
            std::string backupError;
            if (!readLibrary(backup, library, backupError)) {
                throw std::runtime_error("Primary: " + primaryError + "; backup: " + backupError);
            }
            recovered = true;
            Log::Warn("Recovered library from " + backup.string() + "; primary: " + primaryError);
        }

        // Commit to live state only after the entire file has been validated.
        m_Books = std::move(library.books);
        m_NextId = library.nextId;
        sortBooks(BookSortMode::IdAsc);
        loadedPositions = std::move(library.positions);
        m_RecoveredPath = recovered ? primary : fs::path{};
        return true;
    }
    catch (const std::exception& exception) {
        setLastError("Load failed: " + std::string(exception.what()));
        Log::Error(getLastError());
        return false;
    }
}
