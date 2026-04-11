#include "date_utils.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <regex>
#include <sstream>

namespace DateUtils {

bool IsValidDateDDMMYYYY(const std::string& text)
{
    if (text.empty()) return true;

    static const std::regex kPattern(R"(^\d{2}\.\d{2}\.\d{4}$)");
    if (!std::regex_match(text, kPattern)) return false;

    const int day = std::stoi(text.substr(0, 2));
    const int month = std::stoi(text.substr(3, 2));
    const int year = std::stoi(text.substr(6, 4));

    if (year < 1900 || year > 3000) return false;
    if (month < 1 || month > 12) return false;

    static const int daysInMonth[12] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };

    int maxDay = daysInMonth[month - 1];
    const bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    if (month == 2 && leap) maxDay = 29;

    return day >= 1 && day <= maxDay;
}

int CompareDateDDMMYYYY(const std::string& lhs, const std::string& rhs)
{
    const int lhsDay = std::stoi(lhs.substr(0, 2));
    const int lhsMonth = std::stoi(lhs.substr(3, 2));
    const int lhsYear = std::stoi(lhs.substr(6, 4));

    const int rhsDay = std::stoi(rhs.substr(0, 2));
    const int rhsMonth = std::stoi(rhs.substr(3, 2));
    const int rhsYear = std::stoi(rhs.substr(6, 4));

    if (lhsYear != rhsYear) return (lhsYear < rhsYear) ? -1 : 1;
    if (lhsMonth != rhsMonth) return (lhsMonth < rhsMonth) ? -1 : 1;
    if (lhsDay != rhsDay) return (lhsDay < rhsDay) ? -1 : 1;
    return 0;
}

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

} // namespace DateUtils
