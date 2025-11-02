#pragma once

#include <string>
#include <vector>
#include "genres.h"
#include "include/nlohmann/json.hpp"

using nlohmann::json;

enum class Status
{
	ToRead,
	Reading,
	Read,
};

inline std::string statusToString(Status status) {
	switch (status) {
	case Status::ToRead: return "ToRead";
	case Status::Reading: return "Reading";
	case Status::Read: return "Read";
	default: return "Unknown";
	}
}

inline Status stringToStatus(const std::string& s) {
	if (s == "ToRead") return Status::ToRead;
	if (s == "Reading") return Status::Reading;
	if (s == "Read") return Status::Read;
	throw std::runtime_error("Unknown Status: " + s);
}
inline void to_json(json& j, const Status& s) {
	j = statusToString(s);
}

inline void from_json(const json& j, Status& s) {
	s = stringToStatus(j.get<std::string>());
}

class Book
{
public:

	Book();
	Book(const std::string& title, const std::string& author, Status status);
	
	static std::string ratingToStars(float rating);

	static std::string statusToString(Status status);

	int getId() const { return m_id; }
	const std::string& getTitle() const { return m_Title; }
	const std::string& getAuthor() const { return m_Author; }
	Status getStatus() const { return m_Status; }
	const std::vector<Genre>& getGenres() const { return m_Genres; }
	const std::string& getNotes() const { return m_Notes; }


	void setId(int id) { m_id = id; }
	void setTitle(const std::string& title) { m_Title = title; }
	void setAuthor(const std::string& author) { m_Author = author; }
	void setStatus(Status status) { m_Status = status; }
	void addGenre(Genre genre) { m_Genres.push_back(genre); }
	void clearGenres() { m_Genres.clear(); } 
	void setNotes(const std::string& notes) { m_Notes = notes; }
	float getRating() const { return m_Rating; }
	
	void setRating(float rating) {
		if (rating >= 0.0f && rating <= 5.0f) {
			m_Rating = rating;
		}
	}

private:
	int m_id;
	std::string m_Title;
	std::string m_Author;
	Status m_Status;
	std::vector<Genre> m_Genres;
	std::string m_Notes;
	float m_Rating;

};

inline void to_json(json& j, const Book& b) {
	j = json{
		{"id", b.getId()},
		{"title", b.getTitle()},
		{"author", b.getAuthor()},
		{"status", b.getStatus()},
		{"genres", b.getGenres()},
		{"notes", b.getNotes()},
		{"rating", b.getRating()}
	};
}

inline void from_json(const json& j, Book& b) {
	b.setId(j.at("id").get<int>());
	b.setTitle(j.at("title").get<std::string>());
	b.setAuthor(j.at("author").get<std::string>());
	b.setStatus(j.at("status").get<Status>());

	// --- Začátek úpravy ---

	// Nejprve vymažeme staré žánry pro případ, že načítáme do existující knihy
	b.clearGenres();

	// Bezpečnější načtení žánrů: pouze pokud existují a jsou pole
	if (j.contains("genres") && j.at("genres").is_array()) {
		// Načteme do dočasného vektoru
		auto genres = j.at("genres").get<std::vector<Genre>>();
		for (const auto& genre : genres) {
			b.addGenre(genre);
		}
	}
	// --- Konec úpravy ---

	b.setNotes(j.at("notes").get<std::string>());
	b.setRating(j.at("rating").get<float>());
}