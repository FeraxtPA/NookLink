
// Manages search and filter queries for books and genres.
// Parses query syntax and evaluates matching conditions.


#pragma once
#include <string>
#include <vector>
#include "book.h"

enum class RuleType { Text, RatingGreater, RatingLower, RatingEqual, Status, Genre, GenreMissing, FinishedRange };

struct FilterRule {
    RuleType type = RuleType::Text;
    float ratingVal = 0.0f;
    std::string stringVal = "";
};

class SearchFilter {
public:
    SearchFilter() = default;

    // Called only when search query text changes
    void setQuery(const std::string& query);

    // Checks if any search/filter is active
    bool isActive() const { return !m_Rules.empty(); }

    // Methods for evaluating specific entities
    bool matchesBook(const Book* book) const;
    bool matchesGenre(const std::string& genreName) const;

private:
    std::string m_Query;
    std::vector<FilterRule> m_Rules;

    void parseQuery();

    
    std::string statusToString(Status s) const;
};