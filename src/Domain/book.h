
// Data model for a single book entry.
// Contains metadata such as title, author, rating, status, and reading dates.
// Provides utility functions for status management and JSON serialization.


#pragma once
#include <string>
#include <vector>

#include "../include/nlohmann/json.hpp"
#include <algorithm>
#include <cmath>

using nlohmann::json;

enum class Status
{
	ToRead,
	Reading,
	Read,
};

// Rounds a rating to two decimal places for consistent JSON serialization
inline float GetRatingTwoDecimal(const float& rating) {
  return std::round(rating * 100.0f) / 100.0f;
}


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
	return Status::ToRead;
}

// Convert Status enum to JSON string
inline void to_json(json& j, const Status& s) {
	j = statusToString(s);
}

// Convert JSON string back to Status enum
inline void from_json(const json& j, Status& s) {
	s = stringToStatus(j.get<std::string>());
}

class Book
{
public:

	Book();
	Book(const std::string& title, const std::string& author, Status status);
	
	static std::string ratingToStars(float rating);

	int getId() const { return m_id; }
	const std::string& getTitle() const { return m_Title; }
	const std::string& getAuthor() const { return m_Author; }
	Status getStatus() const { return m_Status; }
	const std::vector<std::string>& getGenres() const { return m_Genres; }
	const std::string& getNotes() const { return m_Notes; }
 int getPageCount() const { return m_PageCount; }
 const std::string& getDatePublished() const { return m_DatePublished; }
	const std::string& getDateAdded() const { return m_DateAdded; }
	const std::string& getDateStartedReading() const { return m_DateStartedReading; }
	const std::string& getDateFinishedReading() const { return m_DateFinishedReading; }


	void setId(int id) { m_id = id; }
	void setTitle(const std::string& title) { m_Title = title; }
	void setAuthor(const std::string& author) { m_Author = author; }
	void setStatus(Status status) { m_Status = status; }
	void addGenre(const std::string& genre) {
		if (!genre.empty()) m_Genres.push_back(genre);
	}
	void clearGenres() { m_Genres.clear(); } 
	void setNotes(const std::string& notes) { m_Notes = notes; }
  void setPageCount(int pageCount) { m_PageCount = pageCount < 0 ? 0 : pageCount; }
    void setDatePublished(const std::string& published) { m_DatePublished = published; }
	void setDateAdded(const std::string& dateAdded) { m_DateAdded = dateAdded; }
	void setDateStartedReading(const std::string& started) { m_DateStartedReading = started; }
	void setDateFinishedReading(const std::string& finished) { m_DateFinishedReading = finished; }
	float getRating() const { return m_Rating;}
	
	void setRating(float rating) {
		if (rating >= 0.0f && rating <= 5.0f) {
			m_Rating = rating;
		}
	}

	friend void to_json(json& j, const Book& b) {
		j = json{
			{"id", b.m_id},
			{"title", b.m_Title},
			{"author", b.m_Author},
			{"status", b.m_Status},
			{"genres", b.m_Genres}, 
			{"notes", b.m_Notes},
         {"page_count", b.m_PageCount},
         {"date_published", b.m_DatePublished},
			{"rating", b.m_Rating},
			{"date_added", b.m_DateAdded},
			{"date_started_reading", b.m_DateStartedReading},
			{"date_finished_reading", b.m_DateFinishedReading}
		};
	}

	friend void from_json(const json& j, Book& b) {
		b.m_id = j.value("id", 0);
		b.m_Title = j.value("title", std::string{});
		b.m_Author = j.value("author", std::string{});

		const std::string statusText = j.value("status", std::string{"ToRead"});
		b.m_Status = stringToStatus(statusText);

		if (j.contains("genres") && j["genres"].is_array()) {
			b.m_Genres = j["genres"].get<std::vector<std::string>>();
		}
		else {
			b.m_Genres.clear();
		}

		b.m_Notes = j.value("notes", std::string{});
       const int loadedPageCount = j.value("page_count", 0);
		b.m_PageCount = loadedPageCount < 0 ? 0 : loadedPageCount;
       b.m_DatePublished = j.value("date_published", std::string{});
		b.m_Rating = j.value("rating", 0.0f);
		b.m_DateAdded = j.value("date_added", std::string{});
		b.m_DateStartedReading = j.value("date_started_reading", std::string{});
		b.m_DateFinishedReading = j.value("date_finished_reading", std::string{});
	}

private:

	int m_id;
	std::string m_Title;
	std::string m_Author;
	Status m_Status;
	std::vector<std::string> m_Genres;
	std::string m_Notes;
 int m_PageCount;
 std::string m_DatePublished;
	float m_Rating;
	std::string m_DateAdded;
	std::string m_DateStartedReading;
	std::string m_DateFinishedReading;
	

};

