#pragma once

#include <string>

namespace Validation {

std::string TrimCopy(const std::string& input);
bool IsValidPublishedDate(const std::string& text);
bool TryParsePageCount(const std::string& pageText, int& outPageCount);
std::string NormalizeIsbn(const std::string& input);
bool IsValidIsbnFormat(const std::string& input);
bool TryParseRating(const std::string& ratingText, float& outRating);

} // namespace Validation
