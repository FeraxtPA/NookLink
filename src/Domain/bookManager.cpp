
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
	Book newBook = book;
	newBook.setId(m_NextId++);
	m_Books.push_back(newBook);
	
	return newBook.getId();
}

bool BookManager::restoreBook(const Book& book)
{
	if (findBookById(book.getId()) != nullptr) {
		Log::Warn("restoreBook skipped: ID already exists " + std::to_string(book.getId()));
		return false;
	}

	// Keep m_Books ordered by ID so lower_bound lookups stay valid.
	auto it = std::lower_bound(m_Books.begin(), m_Books.end(), book.getId(),
		[](const Book& current, int value) {
			return current.getId() < value;
		});

	m_Books.insert(it, book);
	if (book.getId() >= m_NextId) {
		m_NextId = book.getId() + 1;
	}

	return true;
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
	// Keep lookup cost low for large libraries.
	auto it = std::lower_bound(m_Books.begin(), m_Books.end(), id,
		[](const Book& book, int value) {
			return book.getId() < value;
		});

	if (it != m_Books.end() && it->getId() == id) {
		return &(*it);
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
	case BookSortMode::IdAsc:
	default:
		std::sort(m_Books.begin(), m_Books.end(), [](const Book& a, const Book& b) {
			return a.getId() < b.getId();
		});
		break;
	}
}

bool BookManager::saveBooksToFile(const std::string& filename, const std::unordered_map<int, NodePosition>& positions) const
{
	setLastError("");

	if (filename.empty()) {
		const std::string message = "Save failed: target filename is empty";
		setLastError(message);
		Log::Error(message);
		return false;
	}

	nlohmann::json j;

	j["books"] = m_Books;
	j["next_id"] = m_NextId;

	nlohmann::json posJson = nlohmann::json::object();
	// Persist positions for all graph nodes keyed by node ID.
	for (const auto& [id, pos] : positions) {
		posJson[std::to_string(id)] = { {"x", pos.x}, {"y", pos.y}, {"locked", pos.locked} };
	}
	j["positions"] = posJson;

	const fs::path targetPath(filename);
	const fs::path tempPath = targetPath.string() + ".tmp";
	const fs::path backupPath = targetPath.string() + ".bak";

	std::error_code ec;
	if (fs::exists(targetPath, ec)) {
		fs::copy_file(targetPath, backupPath, fs::copy_options::overwrite_existing, ec);
		if (ec) {
			Log::Warn("Failed to create backup file '" + backupPath.string() + "': " + ec.message());
		}
	}

	std::ofstream o(tempPath, std::ios::trunc);
	if (!o.is_open()) {
		const std::string message = "Could not open temp file for writing: " + tempPath.string();
		setLastError(message);
		Log::Error(message);
		return false;
	}

	// Two-phase replace: write temp first, then atomically move into place.
	o << j.dump(4) << std::endl;
	o.flush();
	o.close();
	if (!o) {
		const std::string message = "Failed while writing temp file: " + tempPath.string();
		setLastError(message);
		Log::Error(message);
		return false;
	}

	ec.clear();
	fs::rename(tempPath, targetPath, ec);
	if (ec) {
		std::error_code removeEc;
		fs::remove(targetPath, removeEc);
		ec.clear();
		fs::rename(tempPath, targetPath, ec);
		if (ec) {
			const std::string message = "Failed to replace target file '" + targetPath.string() + "': " + ec.message();
			setLastError(message);
			Log::Error(message);
			std::error_code cleanupEc;
			fs::remove(tempPath, cleanupEc);
			return false;
		}
	}

	Log::Info("Books and positions successfully saved to " + filename);
	return true;
}
bool BookManager::loadBooksFromFile(const std::string& filename, std::unordered_map<int, NodePosition>& loadedPositions)
{
	loadedPositions.clear();
	setLastError("");

	if (filename.empty()) {
		const std::string message = "Load failed: source filename is empty";
		setLastError(message);
		Log::Error(message);
		return false;
	}

	auto tryLoadFromPath = [&](const fs::path& path, std::vector<Book>& outBooks, int& outNextId, std::unordered_map<int, NodePosition>& outPositions) -> bool {
		std::ifstream i(path);
		if (!i.is_open()) {
			return false;
		}

		nlohmann::json j;
		try {
			j = nlohmann::json::parse(i);
		}
		catch (const nlohmann::json::parse_error& e) {
			setLastError("Failed to parse JSON file '" + path.string() + "': " + e.what());
			Log::Error(getLastError());
			return false;
		}

		try {
			outBooks.clear();
			if (j.contains("books") && j["books"].is_array()) {
				// Best-effort import: malformed entries are skipped, valid ones still load.
				for (const auto& entry : j["books"]) {
					try {
						outBooks.push_back(entry.get<Book>());
					}
					catch (const std::exception& e) {
						Log::Warn("Skipping malformed book entry while loading '" + path.string() + "': " + std::string(e.what()));
					}
				}
			}
			outNextId = j.value("next_id", 1);

			if (j.contains("positions")) {
				for (auto& el : j["positions"].items()) {
					int id = std::stoi(el.key());
					float x = el.value()["x"];
					float y = el.value()["y"];
					bool locked = el.value().value("locked", false);
					outPositions[id] = { x, y, locked };
				}
			}
		}
		catch (const std::exception& e) {
			setLastError("Error reconstructing books from file '" + path.string() + "': " + e.what());
			Log::Error(getLastError());
			return false;
		}

		return true;
	};

	const fs::path primaryPath(filename);
	const fs::path backupPath = primaryPath.string() + ".bak";

	std::vector<Book> newBooks;
	int newNextId = 1;

	if (!tryLoadFromPath(primaryPath, newBooks, newNextId, loadedPositions)) {
		const std::string primaryError = getLastError();
		loadedPositions.clear();
		std::vector<Book> backupBooks;
		int backupNextId = 1;

		// Automatic recovery path: if primary fails, attempt .bak file.
		if (tryLoadFromPath(backupPath, backupBooks, backupNextId, loadedPositions)) {
			Log::Warn("Loaded backup file after primary load failed: " + backupPath.string());
			if (!primaryError.empty()) {
				Log::Warn("Primary load failure reason: " + primaryError);
			}
			newBooks = std::move(backupBooks);
			newNextId = backupNextId;
		}
		else {
			const std::string backupError = getLastError();
			const std::string combinedError =
				"Could not load primary file or backup: " + primaryPath.string() + " / " + backupPath.string() +
				" | primary: " + (primaryError.empty() ? "n/a" : primaryError) +
				" | backup: " + (backupError.empty() ? "n/a" : backupError);
			setLastError(combinedError);
			Log::Error(combinedError);
			loadedPositions.clear();
			return false;
		}
	}

	m_Books = std::move(newBooks);
	std::sort(m_Books.begin(), m_Books.end(), [](const Book& a, const Book& b) {
		return a.getId() < b.getId();
	});
	m_NextId = newNextId;
	return true;
}
