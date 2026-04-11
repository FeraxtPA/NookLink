#pragma once

#include <string>

namespace DateUtils {

bool IsValidDateDDMMYYYY(const std::string& text);
int CompareDateDDMMYYYY(const std::string& lhs, const std::string& rhs);
std::string GetTodayDateDDMMYYYY();

} // namespace DateUtils
