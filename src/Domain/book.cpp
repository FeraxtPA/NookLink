
// Implementation of the Book data model class.
// Provides constructors, initialization, and utility functions for books.


#include "book.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace {
std::string GetTodayDateDDMMYYYY()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t tt = std::chrono::system_clock::to_time_t(now);

    std::tm localTm{};
#if defined(_WIN32)
    localtime_s(&localTm, &tt);
#else
    localtime_r(&tt, &localTm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&localTm, "%d.%m.%Y");
    return oss.str();
}
}

Book::Book()
    : m_id(0),
      m_Title(""),
      m_Author(""),
      m_Status(Status::ToRead),
      m_Notes(""),
      m_PageCount(0),
      m_DatePublished(""),
      m_Rating(0.0f),
      m_DateAdded(GetTodayDateDDMMYYYY()),
      m_DateStartedReading(""),
      m_DateFinishedReading("")
{}


Book::Book(const std::string& title, const std::string& author, Status status) :
	m_Title(title), m_Author(author), m_Status(status)
{
	m_id = 0; 
	m_Notes = "";
   m_PageCount = 0;
   m_DatePublished = "";
	m_Rating = 0.0f; 
    m_DateAdded = GetTodayDateDDMMYYYY();
    m_DateStartedReading = "";
    m_DateFinishedReading = "";

}

std::string Book::ratingToStars(float rating)
{
    if (rating < 0.0f)
        return "Unrated";

    // Ensure rating does not exceed 5.0
    float clampedRating = std::min(rating, 5.0f);

    // Get the whole number of stars
    int full = static_cast<int>(clampedRating);

    // Get the fractional part
    float fraction = clampedRating - full;

    std::string fractionalStar;
    int fractionalValue = 0; // 0: none, 1: 1/4, 2: 1/2, 3: 3/4

    if (fraction >= 0.25f && fraction < 0.50f) {
        fractionalStar = "[\xC2\xBC]"; // 1/4
        fractionalValue = 1;
    }
    else if (fraction >= 0.50f && fraction < 0.75f) {
        fractionalStar = "[\xC2\xBD]"; // 1/2
        fractionalValue = 1;
    }
    else if (fraction >= 0.75f) {
        fractionalStar = "[\xC2\xBE]"; // 3/4
        fractionalValue = 1;
    }

    // Calculate the number of empty stars needed
    int empty = 5 - full - fractionalValue;

    std::string bar;

    // Full stars (★)
    for (int i = 0; i < full; i++) bar += "[\xE2\x98\x85]";

    // Fractional star (¼, ½, or ¾)
    if (fractionalValue > 0) bar += fractionalStar;

    // Empty stars ([ ])
    for (int i = 0; i < empty; i++) bar += "[ ]";

    return bar;
}




