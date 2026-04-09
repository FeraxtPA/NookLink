#pragma once
#include <string>
#include <vector>
#include "book.h"

// Typy pravidel vy�len�n� z GraphManageru
enum class RuleType { Text, RatingGreater, RatingLower, RatingEqual, Status, Genre, FinishedRange };

struct FilterRule {
    RuleType type = RuleType::Text;
    float ratingVal = 0.0f;
    std::string stringVal = "";
};

class SearchFilter {
public:
    SearchFilter() = default;

    // Zavol� se POUZE kdy� se zm�n� text ve vyhled�v�n�
    void setQuery(const std::string& query);

    // Zjist�, zda je v�bec n�jak� vyhled�v�n� aktivn�
    bool isActive() const { return !m_Rules.empty(); }

    // �ist� metody pro vyhodnocen� konkr�tn�ch entit
    bool matchesBook(const Book* book) const;
    bool matchesGenre(const std::string& genreName) const;

private:
    std::string m_Query;
    std::vector<FilterRule> m_Rules;

    void parseQuery();

    // Pomocn� funkce p�esunut� z graphManager.cpp
    std::string statusToString(Status s) const;
};