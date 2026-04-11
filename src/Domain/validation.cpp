#include "validation.h"

#include <algorithm>
#include <cctype>
#include <regex>

namespace Validation {

std::string TrimCopy(const std::string& input)
{
    auto begin = std::find_if_not(input.begin(), input.end(), [](unsigned char c) { return std::isspace(c); });
    auto end = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char c) { return std::isspace(c); }).base();

    if (begin >= end) {
        return "";
    }

    return std::string(begin, end);
}

bool IsValidPublishedDate(const std::string& text)
{
    if (text.empty()) return true;

    static const std::regex kPatternYear(R"(^\d{4}$)");
    static const std::regex kPatternYearMonth(R"(^\d{4}-\d{2}$)");
    static const std::regex kPatternYearMonthDay(R"(^\d{4}-\d{2}-\d{2}$)");

    if (std::regex_match(text, kPatternYear)) {
        return true;
    }

    if (std::regex_match(text, kPatternYearMonth)) {
        const int month = std::stoi(text.substr(5, 2));
        return month >= 1 && month <= 12;
    }

    if (std::regex_match(text, kPatternYearMonthDay)) {
        const int month = std::stoi(text.substr(5, 2));
        const int day = std::stoi(text.substr(8, 2));
        return month >= 1 && month <= 12 && day >= 1 && day <= 31;
    }

    return false;
}

bool TryParsePageCount(const std::string& pageText, int& outPageCount)
{
    const std::string trimmed = TrimCopy(pageText);
    if (trimmed.empty()) {
        outPageCount = 0;
        return true;
    }

    for (char ch : trimmed) {
        if (!std::isdigit((unsigned char)ch)) {
            return false;
        }
    }

    try {
        const int parsed = std::stoi(trimmed);
        outPageCount = parsed < 0 ? 0 : parsed;
        return true;
    }
    catch (...) {
        return false;
    }
}

std::string NormalizeIsbn(const std::string& input)
{
    std::string normalized;
    normalized.reserve(input.size());

    for (char ch : input) {
        if (std::isdigit((unsigned char)ch) || ch == 'X' || ch == 'x') {
            normalized.push_back((char)std::toupper((unsigned char)ch));
        }
    }

    return normalized;
}

bool IsValidIsbnFormat(const std::string& input)
{
    const std::string normalized = NormalizeIsbn(input);
    if (normalized.size() != 10 && normalized.size() != 13) {
        return false;
    }

    for (size_t i = 0; i < normalized.size(); ++i) {
        const char ch = normalized[i];
        if (i == normalized.size() - 1 && normalized.size() == 10 && ch == 'X') {
            continue;
        }

        if (!std::isdigit((unsigned char)ch)) {
            return false;
        }
    }

    return true;
}

bool TryParseRating(const std::string& ratingText, float& outRating)
{
    const std::string trimmed = TrimCopy(ratingText);
    if (trimmed.empty()) {
        outRating = 0.0f;
        return true;
    }

    try {
        size_t processed = 0;
        const float parsed = std::stof(trimmed, &processed);
        if (processed != trimmed.size()) {
            return false;
        }
        outRating = parsed;
        return true;
    }
    catch (...) {
        return false;
    }
}

} // namespace Validation
