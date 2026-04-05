#include "searchFilter.h"
#include <sstream>
#include <algorithm>
#include <cmath>

void SearchFilter::setQuery(const std::string& query) {
    if (m_Query == query) return; // Nemusíme parsovat, pokud se nic nezmìnilo

    m_Query = query;
    m_Rules.clear();
    parseQuery();
}

void SearchFilter::parseQuery() {
    if (m_Query.empty()) return;

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
        else {
            rule.type = RuleType::Text;
            rule.stringVal = segment;
        }

        m_Rules.push_back(rule);
    }
}

bool SearchFilter::matchesBook(const Book* book) const {
    if (!book) return false;

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

        // Pokud kniha nesplòuje by JEDNO pravidlo v øetìzci (oddìleném |), celá kniha je vyøazena
        if (!ruleMatch) return false;
    }
    return true;
}

bool SearchFilter::matchesGenre(const std::string& genreName) const {
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