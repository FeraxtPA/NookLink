
// Implementation of the SearchFilter class.
// Parses search queries and evaluates filter conditions for books and genres.


#include "searchFilter.h"
#include "validation.h"
#include "date_utils.h"
#include <sstream>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <cctype>

namespace {
bool TryParseDateDDMMYYYY(const std::string& text, int& outDay, int& outMonth, int& outYear)
{
    if (text.size() != 10) {
        return false;
    }

    const char sep = text[2];
    if (!((sep == '.') || (sep == '-') || (sep == '/')) || text[5] != sep) {
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
    auto normalized = text;
    normalized[2] = normalized[5] = '.';
    return DateUtils::IsValidDateDDMMYYYY(normalized);
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
        segment = Validation::TrimCopy(segment);
        if (segment.empty()) continue;

        // Convert to Lowercase
        std::transform(segment.begin(), segment.end(), segment.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

        FilterRule rule;

        // Determine Rule Type
        if (segment.starts_with("r>") || segment.starts_with("rating>")) {
            rule.type = RuleType::RatingGreater;
            size_t pos = segment.find('>');
            const auto value = Validation::TrimCopy(segment.substr(pos + 1));
            if (value.empty() || !Validation::TryParseRating(value, rule.ratingVal)) rule.type = RuleType::Invalid;
        }
        else if (segment.starts_with("r<") || segment.starts_with("rating<")) {
            rule.type = RuleType::RatingLower;
            size_t pos = segment.find('<');
            const auto value = Validation::TrimCopy(segment.substr(pos + 1));
            if (value.empty() || !Validation::TryParseRating(value, rule.ratingVal)) rule.type = RuleType::Invalid;
        }
        else if (segment.starts_with("r=") || segment.starts_with("rating=") ||
            segment.starts_with("r:") || segment.starts_with("rating:")) {
            rule.type = RuleType::RatingEqual;
            size_t pos = segment.find('=');
            if (pos == std::string::npos) pos = segment.find(':');
            const auto value = Validation::TrimCopy(segment.substr(pos + 1));
            if (value.empty() || !Validation::TryParseRating(value, rule.ratingVal)) rule.type = RuleType::Invalid;
        }
        else if (segment.starts_with("s:")) {
            rule.type = RuleType::Status;
            size_t pos = segment.find(':');
            rule.stringVal = segment.substr(pos + 1);
            if (rule.stringVal.find_first_not_of(" ") != std::string::npos)
                rule.stringVal.erase(0, rule.stringVal.find_first_not_of(" "));
        }
        else if (segment.starts_with("g:")) {
            size_t pos = segment.find(':');
            rule.stringVal = segment.substr(pos + 1);
            if (rule.stringVal.find_first_not_of(" ") != std::string::npos)
                rule.stringVal.erase(0, rule.stringVal.find_first_not_of(" "));

            if (rule.stringVal == "none" || rule.stringVal == "missing" || rule.stringVal == "empty") {
                rule.type = RuleType::GenreMissing;
            }
            else {
                rule.type = RuleType::Genre;
            }
        }
        else if (segment.starts_with("fr:") || segment.starts_with("finished:")) {
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
            std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            ruleMatch = s == rule.stringVal ||
                (book->getStatus() == Status::ToRead && rule.stringVal == "toread");
        }
        else if (rule.type == RuleType::Genre) {
            for (const auto& genreStr : book->getGenres()) {
                std::string g = genreStr;
                std::transform(g.begin(), g.end(), g.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
                if (g.find(rule.stringVal) != std::string::npos) {
                    ruleMatch = true;
                    break;
                }
            }
        }
        else if (rule.type == RuleType::GenreMissing) {
            ruleMatch = book->getGenres().empty();
        }
        else if (rule.type == RuleType::Text) {
            std::string t = book->getTitle();
            std::transform(t.begin(), t.end(), t.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            std::string a = book->getAuthor();
            std::transform(a.begin(), a.end(), a.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
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
            std::transform(g.begin(), g.end(), g.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            if (g.find(rule.stringVal) != std::string::npos) ruleMatch = true;
        }
        else if (rule.type == RuleType::GenreMissing) {
            // Missing-genre rule applies only to books, not to genre nodes.
            ruleMatch = false;
        }
        else if (rule.type != RuleType::Invalid) {
            continue;
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
