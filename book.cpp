#include "book.h"

Book::Book() : m_id(0), m_Title(""), m_Author(""), m_Status(Status::ToRead), m_Notes(""), m_Rating(0.0f) {}


Book::Book(const std::string& title, const std::string& author, Status status) :
	m_Title(title), m_Author(author), m_Status(status)
{
	m_id = 0; 
	m_Notes = "";
	m_Rating = 0.0f; 

}

std::string Book::ratingToStars(float rating)
{
		int full = static_cast<int>(rating);
		if (full < 0.0)
			return "Unrated";
		bool half = (rating - full) >= 0.25f && (rating - full) < 0.75f;
		int empty = 5 - full - (half ? 1 : 0);

		std::string bar;
		for (int i = 0; i < full; i++) bar += "[\xE2\x98\x85]";
		if (half) bar += "[\xC2\xBD]";
		for (int i = 0; i < empty; i++) bar += "[ ]";

		return bar;

}

std::string Book::statusToString(Status status)
{
	switch (status) {
	case Status::ToRead: return "To Read";
	case Status::Reading: return "Reading";
	case Status::Read: return "Read";
	default: return "Unknown";
	}
}



