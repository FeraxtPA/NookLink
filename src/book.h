#pragma once
#include <string>
#include <vector>

#include "../include/nlohmann/json.hpp"
#include <cmath>

using nlohmann::json;

enum class Status
{
	ToRead,
	Reading,
	Read,
};


// Rounds a  rating to two decimal places and returns, for saving to json so it looks better
inline float GetRatingTwoDecimal(const float& rating) {

  int i;
  float d_rating = static_cast<double>(rating);

  if (static_cast<double>(d_rating) >= 0)

    i = static_cast<int>(d_rating * 100 + 0.5);

  else

    i = static_cast<int>(d_rating * 100 - 0.5);


  return (i / 100.0);

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
	throw std::runtime_error("Unknown Status: " + s);
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


	void setId(int id) { m_id = id; }
	void setTitle(const std::string& title) { m_Title = title; }
	void setAuthor(const std::string& author) { m_Author = author; }
	void setStatus(Status status) { m_Status = status; }
	void addGenre(const std::string& genre) {
		if (!genre.empty()) m_Genres.push_back(genre);
	}
	void clearGenres() { m_Genres.clear(); } 
	void setNotes(const std::string& notes) { m_Notes = notes; }
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
			{"rating", b.m_Rating}
		};
	}

	friend void from_json(const json& j, Book& b) {
		j.at("id").get_to(b.m_id);
		j.at("title").get_to(b.m_Title);
		j.at("author").get_to(b.m_Author);
		j.at("status").get_to(b.m_Status);
		j.at("genres").get_to(b.m_Genres); 
		if (j.contains("notes")) j.at("notes").get_to(b.m_Notes);
		j.at("rating").get_to(b.m_Rating);
	}

private:

	int m_id;
	std::string m_Title;
	std::string m_Author;
	Status m_Status;
	std::vector<std::string> m_Genres;
	std::string m_Notes;
	float m_Rating;

};

