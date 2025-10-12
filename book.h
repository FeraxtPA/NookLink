#pragma once

#include <string>
#include <vector>
#include "genres.h"

enum class Status
{
	ToRead,
	Reading,
	Read,
};


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