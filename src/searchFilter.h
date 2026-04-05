#pragma once
#include <string>
#include <vector>
#include "book.h"

// Typy pravidel vyèlenìné z GraphManageru
enum class RuleType { Text, RatingGreater, RatingLower, RatingEqual, Status, Genre };

struct FilterRule {
    RuleType type = RuleType::Text;
    float ratingVal = 0.0f;
    std::string stringVal = "";
};

class SearchFilter {
public:
    SearchFilter() = default;

    // Zavolá se POUZE když se zmìní text ve vyhledávání
    void setQuery(const std::string& query);

    // Zjistí, zda je vùbec nìjaké vyhledávání aktivní
    bool isActive() const { return !m_Rules.empty(); }

    // Èisté metody pro vyhodnocení konkrétních entit
    bool matchesBook(const Book* book) const;
    bool matchesGenre(const std::string& genreName) const;

private:
    std::string m_Query;
    std::vector<FilterRule> m_Rules;

    void parseQuery();

    // Pomocná funkce pøesunutá z graphManager.cpp
    std::string statusToString(Status s) const;
};