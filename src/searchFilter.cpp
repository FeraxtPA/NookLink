
// Implementation of the SearchFilter class.
// Parses search queries and evaluates filter conditions for books and genres.


#include "searchFilter.h"
#include <sstream>
#include <algorithm>
#include <cmath>
#include <ctime>

namespace {
bool TryParseDateDDMMYYYY(const std::string& text, int& outDay, int& outMonth, int& outYear)
{
    if (text.size() != 10 || text[2] != '.' || text[5] != '.') {
        return false;
    }

    for (size_t i = 0; i < text.size(); ++i) {
        if (i == 2 || i == 5) {
            continue;
        }
        if (text[i] < '0' || text[i] > '9') {
            return false;
        }
    }

    outDay = std::stoi(text.substr(0, 2));
    outMonth = std::stoi(text.substr(3, 2));
    outYear = std::stoi(text.substr(6, 4));
    return true;
}
}

void SearchFilter::setQuery(const std::string& query) {
    if (m_Query == query) return; 

    // Rebuild parsed rules only when input actually changed.
    m_Query = query;
    m_Rules.clear();
    parseQuery();
}

void SearchFilter::parseQuery() {
    if (m_Query.empty()) return;

    // Mini-language format: rules separated by '|', each rule can be text or prefixed operator.
    std::stringstream ss(m_Query);
    std::string segment;

    while (std::getline(ss, segment, '|')) {
        // Trim Whitespace
        size_t first = segment.find_first_not_of(" ");
        if (first == std::string::npos) continue; // Skip empty segments
        size_t last = segment.find_last_not_of(" ");
        segment = segment.substr(first, (last - first + 1));

        // Convert to Lowercase
        std::transform(segment.begin(), segment.end(), segment.begin(), ::tolower);

        FilterRule rule;

        // Determine Rule Type
        if (segment.find("r>") != std::string::npos || segment.find("rating>") != std::string::npos) {
            rule.type = RuleType::RatingGreater;
            size_t pos = segment.find('>');
            try { rule.ratingVal = std::stof(segment.substr(pos + 1)); }
            catch (...) {}
        }
        else if (segment.find("r<") != std::string::npos || segment.find("rating<") != std::string::npos) {
            rule.type = RuleType::RatingLower;
            size_t pos = segment.find('<');
            try { rule.ratingVal = std::stof(segment.substr(pos + 1)); }
            catch (...) { rule.ratingVal = 5.0f; }
        }
        else if (segment.find("r=") != std::string::npos || segment.find("rating=") != std::string::npos ||
            segment.find("r:") != std::string::npos || segment.find("rating:") != std::string::npos) {
            rule.type = RuleType::RatingEqual;
            size_t pos = segment.find('=');
            if (pos == std::string::npos) pos = segment.find(':');
            try { rule.ratingVal = std::stof(segment.substr(pos + 1)); }
            catch (...) {}
        }
        else if (segment.find("s:") != std::string::npos) {
            rule.type = RuleType::Status;
            size_t pos = segment.find(':');
            rule.stringVal = segment.substr(pos + 1);
            if (rule.stringVal.find_first_not_of(" ") != std::string::npos)
                rule.stringVal.erase(0, rule.stringVal.find_first_not_of(" "));
        }
        else if (segment.find("g:") != std::string::npos) {
            rule.type = RuleType::Genre;
            size_t pos = segment.find(':');
            rule.stringVal = segment.substr(pos + 1);
            if (rule.stringVal.find_first_not_of(" ") != std::string::npos)
                rule.stringVal.erase(0, rule.stringVal.find_first_not_of(" "));
        }
        else if (segment.find("fr:") != std::string::npos || segment.find("finished:") != std::string::npos) {
            rule.type = RuleType::FinishedRange;
            size_t pos = segment.find(':');
            rule.stringVal = segment.substr(pos + 1);
            if (rule.stringVal.find_first_not_of(" ") != std::string::npos)
                rule.stringVal.erase(0, rule.stringVal.find_first_not_of(" "));
        }
        else {
            rule.type = RuleType::Text;
            rule.stringVal = segment;
        }

        m_Rules.push_back(rule);
    }
}

bool SearchFilter::matchesBook(const Book* book) const {
    if (!book) return false;

    // AND semantics: every rule must match for the book to be included.
    for (const auto& rule : m_Rules) {
        bool ruleMatch = false;

        if (rule.type == RuleType::RatingGreater) {
            if (book->getRating() >= rule.ratingVal) ruleMatch = true;
        }
        else if (rule.type == RuleType::RatingLower) {
            if (book->getRating() <= rule.ratingVal) ruleMatch = true;
        }
        else if (rule.type == RuleType::RatingEqual) {
            if (std::abs(book->getRating() - rule.ratingVal) < 0.01f) ruleMatch = true;
        }
        else if (rule.type == RuleType::Status) {
            std::string s = statusToString(book->getStatus());
            std::transform(s.begin(), s.end(), s.begin(), ::tolower);
            if (s.find(rule.stringVal) != std::string::npos) ruleMatch = true;
        }
        else if (rule.type == RuleType::Genre) {
            for (const auto& genreStr : book->getGenres()) {
                std::string g = genreStr;
                std::transform(g.begin(), g.end(), g.begin(), ::tolower);
                if (g.find(rule.stringVal) != std::string::npos) {
                    ruleMatch = true;
                    break;
                }
            }
        }
        else if (rule.type == RuleType::Text) {
            std::string t = book->getTitle();
            std::transform(t.begin(), t.end(), t.begin(), ::tolower);
            std::string a = book->getAuthor();
            std::transform(a.begin(), a.end(), a.begin(), ::tolower);
            if (t.find(rule.stringVal) != std::string::npos || a.find(rule.stringVal) != std::string::npos) {
                ruleMatch = true;
            }
        }
        else if (rule.type == RuleType::FinishedRange) {
            const std::string dateFinished = book->getDateFinishedReading();
            int day = 0;
            int month = 0;
            int year = 0;
            if (TryParseDateDDMMYYYY(dateFinished, day, month, year)) {
                // Compare against current local month/year for relative filters.
                const std::time_t now = std::time(nullptr);
                std::tm localTm{};
#if defined(_WIN32)
                localtime_s(&localTm, &now);
#else
                localtime_r(&now, &localTm);
#endif

                const int currentMonth = localTm.tm_mon + 1;
                const int currentYear = localTm.tm_year + 1900;

                if (rule.stringVal == "month") {
                    ruleMatch = (month == currentMonth && year == currentYear);
                }
                else if (rule.stringVal == "year") {
                    ruleMatch = (year == currentYear);
                }
                else if (rule.stringVal == "all") {
                    ruleMatch = true;
                }
            }
        }

        if (!ruleMatch) return false;
    }
    return true;
}

bool SearchFilter::matchesGenre(const std::string& genreName) const {
    // Genre matching uses only text/genre rules; numeric/status rules are book-only.
    for (const auto& rule : m_Rules) {
        bool ruleMatch = false;

        if (rule.type == RuleType::Genre || rule.type == RuleType::Text) {
            std::string g = genreName;
            std::transform(g.begin(), g.end(), g.begin(), ::tolower);
            if (g.find(rule.stringVal) != std::string::npos) ruleMatch = true;
        }

        if (!ruleMatch) return false;
    }
    return true;
}

std::string SearchFilter::statusToString(Status s) const {
    switch (s) {
    case Status::ToRead: return "to read";
    case Status::Reading: return "reading";
    case Status::Read: return "read";
    default: return "";
    }
}