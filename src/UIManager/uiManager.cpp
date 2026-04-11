
// Core UI management implementation.
// Handles UI initialization, calendar management, and main UI operations.


#include "uiManager.h"

#include "bookManager.h"
#include "graphManager.h"
#include "UI/button.h"
#include "UI/calendarWidget.h"
#include "UI/checkbox.h"
#include "UI/dropdown.h"
#include "UI/flexLayout.h"
#include "UI/label.h"
#include "UI/panel.h"
#include "UI/slider.h"
#include "UI/textBox.h"
#include "UI/textInput.h"
#include "UI/widget.h"
#include "uiManager_internal.h"

#include <format>
#include <cctype>
#include <ctime>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <unordered_set>

namespace {
std::string TrimCopy(std::string value)
{
    const size_t first = value.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    const size_t last = value.find_last_not_of(" \t\n\r");
    return value.substr(first, last - first + 1);
}

std::string ToLowerCopy(std::string value)
{
    for (char& ch : value) {
        ch = (char)std::tolower((unsigned char)ch);
    }
    return value;
}

bool ParseDateDDMMYYYY(const std::string& date, int& day, int& month, int& year)
{
    if (date.size() != 10) return false;

    const char sep = date[2];
    if (!((sep == '.') || (sep == '-') || (sep == '/'))) return false;
    if (date[5] != sep) return false;

    auto toInt = [](char a, char b) -> int {
        if (!std::isdigit((unsigned char)a) || !std::isdigit((unsigned char)b)) return -1;
        return (a - '0') * 10 + (b - '0');
    };

    day = toInt(date[0], date[1]);
    month = toInt(date[3], date[4]);
    if (day < 1 || month < 1 || month > 12) return false;

    if (!std::isdigit((unsigned char)date[6]) || !std::isdigit((unsigned char)date[7]) ||
        !std::isdigit((unsigned char)date[8]) || !std::isdigit((unsigned char)date[9])) {
        return false;
    }
    year = (date[6] - '0') * 1000 + (date[7] - '0') * 100 + (date[8] - '0') * 10 + (date[9] - '0');
    return year >= 1900;
}

}

UIManager::UIManager(int screenWidth, int screenHeight)
    : m_ScreenWidth(screenWidth), m_ScreenHeight(screenHeight)
{}

UIManager::~UIManager() {}

std::string UIManager::GetSearchText() const
{
    return m_SearchBar ? m_SearchBar->GetText() : std::string{};
}

bool UIManager::IsSearchBarFocused() const
{
    return m_SearchBar && m_SearchBar->isFocused;
}

void UIManager::SetSearchFocusIndicator(int oneBasedIndex, int totalCount)
{
    m_SearchFocusIndicatorIndex = oneBasedIndex;
    m_SearchFocusIndicatorTotal = totalCount;
}

void UIManager::ClearSearchFocusIndicator()
{
    m_SearchFocusIndicatorIndex = -1;
    m_SearchFocusIndicatorTotal = 0;
}

void UIManager::MarkAnalyticsDirty()
{
    m_AnalyticsDirty = true;
    m_AnalyticsRefreshTimer = 0.0f;
}

void UIManager::OnWindowResize(int width, int height)
{
    m_ScreenWidth = width;
    m_ScreenHeight = height;
    if (m_ToolbarLayout) {
        m_ToolbarLayout->SetSize({ std::max(0.0f, (float)width - UiMetrics::kToolbarWidthInset), UiMetrics::kToolbarRowHeight });
    }
    for (auto& w : m_Widgets) {
        w->OnWindowResize(width, height);
    }

    if (m_AnalyticsPanel) {
        const float panelWidth = std::clamp((float)m_ScreenWidth * 0.82f, 900.0f, 1700.0f);
        const float panelHeight = std::clamp((float)m_ScreenHeight * 0.78f, 700.0f, 980.0f);
        m_AnalyticsPanel->SetSize({ panelWidth, panelHeight });
        m_AnalyticsPanel->OnWindowResize(width, height);
    }
}

bool Widget::s_LeftClickConsumed = false;

void UIManager::UpdateAnalyticsPanelTexts(const BookManager& bookManager)
{
    auto Ellipsize = [](const std::string& value, size_t maxLen) {
        if (value.size() <= maxLen) return value;
        if (maxLen <= 3) return value.substr(0, maxLen);
        return value.substr(0, maxLen - 3) + "...";
    };
    auto ExtractPublishedYear = [](const std::string& published) -> int {
        if (published.size() < 4) return 0;
        if (!std::isdigit((unsigned char)published[0]) ||
            !std::isdigit((unsigned char)published[1]) ||
            !std::isdigit((unsigned char)published[2]) ||
            !std::isdigit((unsigned char)published[3])) {
            return 0;
        }
        const int year = (published[0] - '0') * 1000 +
            (published[1] - '0') * 100 +
            (published[2] - '0') * 10 +
            (published[3] - '0');
        return year >= 1000 ? year : 0;
    };

    const auto& books = bookManager.getBooks();
    const int totalBooks = (int)books.size();

    int toReadCount = 0;
    int readingCount = 0;
    int readCount = 0;

    int ratedCount = 0;
    float ratingSum = 0.0f;
    std::array<int, 5> ratingBins{};

    std::unordered_map<std::string, int> genreCounts;
    std::unordered_map<std::string, int> authorCounts;
    std::map<int, int> publishedByDecade;
    std::vector<std::pair<std::string, float>> ratedBooks;
    int finishedThisMonth = 0;
    int finishedThisYear = 0;
    int booksWithPages = 0;
    int totalPages = 0;
    const int pageBinCount = std::clamp(m_AnalyticsPageBinCount, 4, 10);
    std::vector<int> pageBins((size_t)pageBinCount, 0);
    std::vector<std::pair<int, int>> pageBinRanges((size_t)pageBinCount, { 0, 0 });
    int maxPages = 0;
    int publishedYearCount = 0;
    int oldestPublishedYear = 0;
    int newestPublishedYear = 0;

    std::time_t nowTime = std::time(nullptr);
    std::tm nowTm{};
#if defined(_WIN32)
    localtime_s(&nowTm, &nowTime);
#else
    localtime_r(&nowTime, &nowTm);
#endif
    const int currentMonth = nowTm.tm_mon + 1;
    const int currentYear = nowTm.tm_year + 1900;

    for (const Book& book : books) {
        const Status status = book.getStatus();
        if (status == Status::ToRead) ++toReadCount;
        else if (status == Status::Reading) ++readingCount;
        else ++readCount;

        const std::string author = TrimCopy(book.getAuthor());
        if (!author.empty()) {
            authorCounts[author]++;
        }

        const float rating = book.getRating();
        if (rating > 0.0f) {
            ratingSum += rating;
            ++ratedCount;
            ratedBooks.push_back({ book.getTitle(), rating });

            const float clampedRating = std::clamp(rating, 0.0f, 4.999f);
            const int bin = std::clamp((int)clampedRating, 0, 4);
            ratingBins[(size_t)bin]++;
        }

        for (const std::string& genre : book.getGenres()) {
            const std::string cleanGenre = TrimCopy(genre);
            if (!cleanGenre.empty()) {
                genreCounts[cleanGenre]++;
            }
        }

        const int pages = book.getPageCount();
        if (pages > 0) {
            ++booksWithPages;
            totalPages += pages;
            maxPages = std::max(maxPages, pages);
        }

        const int publishedYear = ExtractPublishedYear(book.getDatePublished());
        if (publishedYear > 0) {
            ++publishedYearCount;
            oldestPublishedYear = (oldestPublishedYear == 0) ? publishedYear : std::min(oldestPublishedYear, publishedYear);
            newestPublishedYear = std::max(newestPublishedYear, publishedYear);
            const int decade = (publishedYear / 10) * 10;
            publishedByDecade[decade]++;
        }

        int d = 0, m = 0, y = 0;
        if (ParseDateDDMMYYYY(book.getDateFinishedReading(), d, m, y)) {
            if (y == currentYear) {
                ++finishedThisYear;
                if (m == currentMonth) {
                    ++finishedThisMonth;
                }
            }
        }
    }

    float avgRating = 0.0f;
    if (ratedCount > 0) {
        avgRating = ratingSum / (float)ratedCount;
    }
    const float avgPages = booksWithPages > 0 ? (float)totalPages / (float)booksWithPages : 0.0f;

    if (booksWithPages > 0) {
        const int safeMaxPages = std::max(1, maxPages);
        const int binWidth = std::max(1, (safeMaxPages + pageBinCount - 1) / pageBinCount);
        for (int i = 0; i < pageBinCount; ++i) {
            const int start = i * binWidth + 1;
            const int end = (i + 1 == pageBinCount)
                ? safeMaxPages
                : std::min(safeMaxPages, (i + 1) * binWidth);
            pageBinRanges[(size_t)i] = { start, end };
        }

        for (const Book& book : books) {
            const int pages = book.getPageCount();
            if (pages <= 0) continue;
            int index = (pages - 1) / binWidth;
            index = std::clamp(index, 0, pageBinCount - 1);
            pageBins[(size_t)index]++;
        }
    }

    std::vector<std::pair<std::string, int>> topGenres;
    topGenres.reserve(genreCounts.size());
    for (const auto& [genre, count] : genreCounts) {
        topGenres.push_back({ genre, count });
    }
    std::sort(topGenres.begin(), topGenres.end(), [](const auto& a, const auto& b) {
        if (a.second != b.second) return a.second > b.second;
        return ToLowerCopy(a.first) < ToLowerCopy(b.first);
    });

    std::vector<std::pair<std::string, int>> topAuthors;
    topAuthors.reserve(authorCounts.size());
    for (const auto& [author, count] : authorCounts) {
        topAuthors.push_back({ author, count });
    }
    std::sort(topAuthors.begin(), topAuthors.end(), [](const auto& a, const auto& b) {
        if (a.second != b.second) return a.second > b.second;
        return ToLowerCopy(a.first) < ToLowerCopy(b.first);
    });

    std::sort(ratedBooks.begin(), ratedBooks.end(), [](const auto& a, const auto& b) {
        if (std::abs(a.second - b.second) > 0.001f) return a.second > b.second;
        return ToLowerCopy(a.first) < ToLowerCopy(b.first);
    });


    m_AnalyticsTotalBooks = totalBooks;
    m_AnalyticsToReadCount = toReadCount;
    m_AnalyticsReadingCount = readingCount;
    m_AnalyticsReadCount = readCount;
    m_AnalyticsRatedCount = ratedCount;
    m_AnalyticsAvgRating = avgRating;
    m_AnalyticsFinishedThisMonth = finishedThisMonth;
    m_AnalyticsFinishedThisYear = finishedThisYear;
    m_AnalyticsTopGenres = topGenres;
    m_AnalyticsTopAuthors = topAuthors;
    m_AnalyticsTopRatedBooks = ratedBooks;
    m_AnalyticsRatingBins = ratingBins;
    m_AnalyticsBooksWithPages = booksWithPages;
    m_AnalyticsAvgPages = avgPages;
    m_AnalyticsPageBins = pageBins;
    m_AnalyticsPageBinRanges = pageBinRanges;
    m_AnalyticsPublishedYearCount = publishedYearCount;
    m_AnalyticsOldestPublishedYear = oldestPublishedYear;
    m_AnalyticsNewestPublishedYear = newestPublishedYear;
    m_AnalyticsPublishedByDecade.clear();
    m_AnalyticsPublishedByDecade.reserve(publishedByDecade.size());
    for (const auto& [decade, count] : publishedByDecade) {
        m_AnalyticsPublishedByDecade.push_back({ decade, count });
    }

    if (m_AnalyticsOverviewLabel) {
        m_AnalyticsOverviewLabel->SetText(
            std::format(
                "Overview\nTotal books: {}\nRated: {} | Avg: {:.2f}\nFinished month/year: {} / {}",
                m_AnalyticsTotalBooks,
                m_AnalyticsRatedCount,
                m_AnalyticsAvgRating,
                m_AnalyticsFinishedThisMonth,
                m_AnalyticsFinishedThisYear));
    }

    if (m_AnalyticsTopGenresListLabel) {
        std::string text = "Top genres\n";
        if (m_AnalyticsTopGenres.empty()) {
            text += "-";
        }
        else {
            for (size_t i = 0; i < m_AnalyticsTopGenres.size(); ++i) {
                text += std::format("{}. {} ({})", i + 1, Ellipsize(m_AnalyticsTopGenres[i].first, 24), m_AnalyticsTopGenres[i].second);
                if (i + 1 < m_AnalyticsTopGenres.size()) text += "\n";
            }
        }
        m_AnalyticsTopGenresListLabel->SetText(text);
    }

    if (m_AnalyticsTopAuthorsListLabel) {
        std::string text = "Top authors\n";
        if (m_AnalyticsTopAuthors.empty()) {
            text += "-";
        }
        else {
            for (size_t i = 0; i < m_AnalyticsTopAuthors.size(); ++i) {
                text += std::format("{}. {} ({})", i + 1, Ellipsize(m_AnalyticsTopAuthors[i].first, 24), m_AnalyticsTopAuthors[i].second);
                if (i + 1 < m_AnalyticsTopAuthors.size()) text += "\n";
            }
        }
        m_AnalyticsTopAuthorsListLabel->SetText(text);
    }

    if (m_AnalyticsTopRatedListLabel) {
        std::string text = "Top rated books\n";
        if (m_AnalyticsTopRatedBooks.empty()) {
            text += "-";
        }
        else {
            for (size_t i = 0; i < m_AnalyticsTopRatedBooks.size(); ++i) {
                text += std::format("{}. {} ({:.1f})", i + 1, Ellipsize(m_AnalyticsTopRatedBooks[i].first, 24), m_AnalyticsTopRatedBooks[i].second);
                if (i + 1 < m_AnalyticsTopRatedBooks.size()) text += "\n";
            }
        }
        m_AnalyticsTopRatedListLabel->SetText(text);
    }

    if (m_AnalyticsPagesLabel) {
        m_AnalyticsPagesLabel->SetText(
            std::format(
                "Pages\nKnown: {}\nAvg: {:.0f}",
                m_AnalyticsBooksWithPages,
                m_AnalyticsAvgPages));
    }

    if (m_AnalyticsPublicationLabel) {
        if (m_AnalyticsPublishedYearCount == 0) {
            m_AnalyticsPublicationLabel->SetText("Publication\nNo year data\n-");
        }
        else {
            m_AnalyticsPublicationLabel->SetText(
                std::format(
                    "Publication\nKnown: {}\n{}-{}",
                    m_AnalyticsPublishedYearCount,
                    m_AnalyticsOldestPublishedYear,
                    m_AnalyticsNewestPublishedYear));
        }
    }

    
}

void UIManager::RefreshKnownGenresFromBookManager(const BookManager& bookManager)
{
    std::vector<std::string> genres;
    std::unordered_set<std::string> seenLower;

    for (const Book& book : bookManager.getBooks()) {
        for (const std::string& genre : book.getGenres()) {
            const std::string clean = TrimCopy(genre);
            if (clean.empty()) continue;

            const std::string key = ToLowerCopy(clean);
            if (seenLower.insert(key).second) {
                genres.push_back(clean);
            }
        }
    }

    std::sort(genres.begin(), genres.end(), [](const std::string& a, const std::string& b) {
        return ToLowerCopy(a) < ToLowerCopy(b);
    });

    if (genres == m_KnownGenres) {
        return;
    }

    m_KnownGenres = std::move(genres);
    SyncGenreDropdownOptions();
}

void UIManager::SyncGenreDropdownOptions()
{
    std::vector<std::string> options;
    options.reserve(m_KnownGenres.size() + 1);
    options.push_back("Select existing genre...");
    options.insert(options.end(), m_KnownGenres.begin(), m_KnownGenres.end());

    if (m_AddGenreDropdown) {
        m_AddGenreDropdown->SetOptions(options, 0);
    }
    if (m_EditGenreDropdown) {
        m_EditGenreDropdown->SetOptions(options, 0);
    }
}
