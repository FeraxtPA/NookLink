
// Core UI management implementation.
// Handles UI initialization, calendar management, and main UI operations.


#include "uiManager.h"

#include "UI/widget.h"

#include <format>
#include <cctype>
#include <ctime>
#include <unordered_map>
#include <unordered_set>

namespace {
constexpr const char* kFilterToolbarGlyph = "☰";
constexpr const char* kFilterToolbarGlyphActive = "☰*";
constexpr const char* kDetailsDivider = "--------------------------------";

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

std::string FormatDetailSection(const std::string& icon, const std::string& heading, const std::string& body)
{
    return icon + std::string("  ") + heading + "\n" + body + "\n" + kDetailsDivider + "\n\n";
}

bool ParseDateDDMMYYYY(const std::string& date, int& day, int& month, int& year)
{
    if (date.size() != 10 || date[2] != '.' || date[5] != '.') return false;

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

void UIManager::OpenCalendarFor(const std::shared_ptr<TextInput>& targetInput)
{
    if (!targetInput || !m_CalendarWidget) {
        return;
    }

    // Keep a weak editing context so date callbacks know which field to write into.
    m_ActiveDateInput = targetInput;

    const Vector2 preferredTopLeft = {
        targetInput->m_Bounds.x,
        targetInput->m_Bounds.y + targetInput->m_Bounds.height + 8.0f
    };

    m_CalendarWidget->Open(
        targetInput->GetText(),
        [this](const std::string& selectedDate) {
            if (m_ActiveDateInput) {
                m_ActiveDateInput->text = selectedDate;
                m_ActiveDateInput->isFocused = false;
            }
            m_ActiveDateInput.reset();
        },
        preferredTopLeft
    );
}

void UIManager::OpenBookDetails(Book* book)
{
    if (!book || !m_BookDetailsPanel) return;

    m_CurrentDetailsBook = *book;
    m_HasCurrentDetailsBook = true;

    std::string genreStr;
    const auto& genres = book->getGenres();
    for (size_t i = 0; i < genres.size(); ++i) {
        genreStr += genres[i];
        if (i < genres.size() - 1) genreStr += ", ";
    }
    if (genreStr.empty()) {
        genreStr = "-";
    }

    const std::string notes = TrimCopy(book->getNotes()).empty() ? std::string("No notes yet.") : book->getNotes();
    const std::string status = statusToString(book->getStatus());
    const std::string stars = Book::ratingToStars(book->getRating());
    const std::string datesBody =
        "Added    : " + (book->getDateAdded().empty() ? std::string("-") : book->getDateAdded()) + "\n" +
        "Started  : " + (book->getDateStartedReading().empty() ? std::string("-") : book->getDateStartedReading()) + "\n" +
        "Finished : " + (book->getDateFinishedReading().empty() ? std::string("-") : book->getDateFinishedReading());

    std::string fullText;
    fullText.reserve(1024);
    fullText += FormatDetailSection("✦", "Title", book->getTitle());
    fullText += FormatDetailSection("✎", "Author", book->getAuthor());
    fullText += FormatDetailSection("☑", "Status", status);
    fullText += FormatDetailSection("★", "Rating", std::format("{}  ({:.1f}/5.0)", stars, book->getRating()));
    fullText += FormatDetailSection("✧", "Dates", datesBody);
    fullText += FormatDetailSection("➤", "Genres", genreStr);
    fullText += std::string("➜  Notes\n") + notes;

    m_DetailsText->SetText(fullText);
    if (m_BookDetailsPanel) m_BookDetailsPanel->SetTitle("Book Info");
    if (m_DetailsEditBtn) m_DetailsEditBtn->isVisible = true;
    if (m_DetailsCloseBtn) m_DetailsCloseBtn->SetText("Close");

    m_BookDetailsPanel->isVisible = true;
}

void UIManager::OpenGenreDetails(const std::string& genreName, int connectedBooks, const std::vector<std::string>& sampleTitles)
{
    if (!m_BookDetailsPanel || !m_DetailsText) return;

    m_HasCurrentDetailsBook = false;

    std::string preview;
    if (sampleTitles.empty()) {
        preview = "No books currently tagged with this genre.";
    }
    else {
        const size_t previewCount = std::min<size_t>(sampleTitles.size(), 6);
        for (size_t i = 0; i < previewCount; ++i) {
            preview += std::format("{:>2}. {}\n", i + 1, sampleTitles[i]);
        }
        if (sampleTitles.size() > previewCount) {
            preview += std::format("... and {} more", sampleTitles.size() - previewCount);
        }
    }

    std::string fullText;
    fullText.reserve(720);
    fullText += FormatDetailSection("➤", "Genre", genreName);
    fullText += FormatDetailSection("✧", "Connected books", std::to_string(connectedBooks));
    fullText += "▶  Preview\n" + preview;

    m_DetailsText->SetText(fullText);
    if (m_BookDetailsPanel) m_BookDetailsPanel->SetTitle("Genre Info");
    if (m_DetailsEditBtn) m_DetailsEditBtn->isVisible = false;
    if (m_DetailsCloseBtn) m_DetailsCloseBtn->SetText("Close");
    m_BookDetailsPanel->isVisible = true;
}

void UIManager::OpenEditPanel(Book* book)
{
    if (!book || !m_EditPanel) return;

    m_EditingBookId = book->getId();

    m_EditTitle->text = book->getTitle();
    m_EditAuthor->text = book->getAuthor();

    
    m_EditRating->text = std::format("{:.2f}", book->getRating());

    
    m_EditNotes->SetText(book->getNotes());

    // Genres
    std::string genreStr = "";
    const auto& genres = book->getGenres();
    for (size_t i = 0; i < genres.size(); ++i) {
        genreStr += genres[i];
        if (i < genres.size() - 1) genreStr += ", ";
    }
    m_EditGenres->text = genreStr;
    if (m_EditGenreDropdown) m_EditGenreDropdown->SetSelectedIndex(0, false);
    if (m_EditStartedDate) m_EditStartedDate->text = book->getDateStartedReading();
    if (m_EditFinishedDate) m_EditFinishedDate->text = book->getDateFinishedReading();

    // Status
    *m_EditStatusState = (int)book->getStatus();
    if (*m_EditStatusState == 0) m_EditStatusBtn->SetText("Status: To Read");
    else if (*m_EditStatusState == 1) m_EditStatusBtn->SetText("Status: Reading");
    else m_EditStatusBtn->SetText("Status: Read");

    m_EditPanel->isVisible = true;
}


MouseCursor Widget::DesiredCursor = MOUSE_CURSOR_DEFAULT;

void UIManager::Update(Vector2 worldMousePos, Vector2 mousePos, BookManager& bookManager, GraphManager* graphRenderer, TextRenderer* textRenderer)
{


    
    m_LastMousePos = mousePos;
    Widget::DesiredCursor = MOUSE_CURSOR_DEFAULT;
    RefreshKnownGenresFromBookManager(bookManager);
    UpdateAnalyticsPanelTexts(bookManager);

    // Notification visibility is time-based and decremented per frame.
    if (m_NotificationTimer > 0.0f) {
        m_NotificationTimer -= GetFrameTime();
    }

  
    if (graphRenderer != nullptr) {
        Node* newlyHovered = graphRenderer->getNodeAtPosition(worldMousePos);
        const int newHoveredId = newlyHovered ? newlyHovered->id : -1;
        const NodeType newHoveredType = newlyHovered ? newlyHovered->type : NodeType::Book;

        if (newHoveredId != m_LastHoveredNodeId || newHoveredType != m_LastHoveredNodeType) {
            m_LastHoveredNodeId = newHoveredId;
            m_LastHoveredNodeType = newHoveredType;
            m_HoverStartTime = GetTime();
            m_CachedTooltipText.clear();
        }
    }

    // Merge free-text search and advanced filter query into a single parser input.
    if (graphRenderer) {
        std::string finalQuery = m_SearchBar->GetText(); 

        if (!m_ActiveFilterQuery.empty()) {
            if (!finalQuery.empty()) finalQuery += " | "; 
            finalQuery += m_ActiveFilterQuery;            
        }

        graphRenderer->setSearchQuery(finalQuery);
    }

    if (m_ToggleFiltersBtn) {
        const bool hasAdvancedFilters = !m_ActiveFilterQuery.empty();
        m_ToggleFiltersBtn->SetText(hasAdvancedFilters ? kFilterToolbarGlyphActive : kFilterToolbarGlyph);
    }

    UpdateGoalPanelTexts();


    
    // Lottery uses a short timed animation before selecting the final winner.
    if (m_IsLotteryRolling)
    {
        // animation
        float dt = GetFrameTime();
        m_LotteryTimer -= dt;
        m_LotterySpeedTimer -= dt;

        if (m_LotterySpeedTimer <= 0.0f && m_LotteryTimer > 0.0f) {
            m_LotterySpeedTimer = 0.05f;
            const auto& allBooks = bookManager.getBooksToBeRead();
            if (!allBooks.empty()) {
                int r = GetRandomValue(0, (int)allBooks.size() - 1);
                m_LotteryText->SetText("... " + allBooks[r].getTitle() + " ...");
            }
        }

        // picking winner
        if (m_LotteryTimer <= 0.0f) {
            m_IsLotteryRolling = false;

            try {
                const Book& winnerConst = bookManager.getRandomBookToBeRead();
                m_LotteryWinnerId = winnerConst.getId(); 
                m_LastLotteryCheckState = m_LotteryAutoRead->checked; 

                if (m_LotteryAutoRead->checked) {
                    Book* winnerMutable = bookManager.getBookById(m_LotteryWinnerId);
                    if (winnerMutable) winnerMutable->setStatus(Status::Reading);
                    if (graphRenderer) graphRenderer->initializePositions();
                }

                //Initial text
                std::string statusMsg = m_LotteryAutoRead->checked ? "\n(Status updated to Reading!)" : "";
                m_LotteryText->SetText(
                    "WINNER!\n\n" +
                    winnerConst.getTitle() + "\n" +
                    "by " + winnerConst.getAuthor() + "\n" +
                    statusMsg
                );
            }
            catch (const std::exception& e) {
                m_LotteryText->SetText("No books found with status 'To Read'!");
                m_LotteryWinnerId = -1;
            }
            m_LotteryCloseBtn->isVisible = true;
        }
    }
    
    // Keep winner status and UI text synchronized with the auto-read checkbox.
    else if (m_LotteryPanel->isVisible && m_LotteryWinnerId != -1)
    {
        
        if (m_LotteryAutoRead->checked != m_LastLotteryCheckState)
        {
            m_LastLotteryCheckState = m_LotteryAutoRead->checked; 

            Book* winner = bookManager.getBookById(m_LotteryWinnerId);
            if (winner) {
                
                //update status
                if (m_LotteryAutoRead->checked) {
                    winner->setStatus(Status::Reading);
                }
                else {
                    winner->setStatus(Status::ToRead); // revert if not checked
                }

                
                if (graphRenderer) graphRenderer->initializePositions();

                // Maybe useless aswell since its set earlier needs testing
                std::string statusMsg = m_LotteryAutoRead->checked ? "\n(Status updated to Reading!)" : "";
                m_LotteryText->SetText(
                    "WINNER!\n\n" +
                    winner->getTitle() + "\n" +
                    "by " + winner->getAuthor() + "\n" +
                    statusMsg
                );
            }
        }
    }

    
    // We evaluate hover both before and after Update() because widgets may move/focus mid-frame.
    bool hoverBeforeUpdate = IsMouseOverUI();

   
    for (auto it = m_Widgets.rbegin(); it != m_Widgets.rend(); ++it) {
        (*it)->Update();
    }

    if (m_CalendarWidget && !m_CalendarWidget->IsOpen()) {
        m_ActiveDateInput.reset();
    }

    isBlockingGraph = hoverBeforeUpdate || IsMouseOverUI();

    if (!isBlockingGraph) {
        UpdateTooltipCache(graphRenderer, bookManager, textRenderer);
        SetMouseCursor(Widget::DesiredCursor);
    }
}

void UIManager::UpdateAnalyticsPanelTexts(const BookManager& bookManager)
{
    if (!m_AnalyticsSummaryLabel || !m_AnalyticsRatingLabel || !m_AnalyticsStatusLabel || !m_AnalyticsTimeLabel || !m_AnalyticsGenreLabel) {
        return;
    }

    const auto& books = bookManager.getBooks();
    const int totalBooks = (int)books.size();

    int toReadCount = 0;
    int readingCount = 0;
    int readCount = 0;

    int ratedCount = 0;
    float ratingSum = 0.0f;

    std::unordered_map<std::string, int> genreCounts;
    int finishedThisMonth = 0;
    int finishedThisYear = 0;

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

        const float rating = book.getRating();
        if (rating > 0.0f) {
            ratingSum += rating;
            ++ratedCount;
        }

        for (const std::string& genre : book.getGenres()) {
            const std::string cleanGenre = TrimCopy(genre);
            if (!cleanGenre.empty()) {
                genreCounts[cleanGenre]++;
            }
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

    std::vector<std::pair<std::string, int>> topGenres;
    topGenres.reserve(genreCounts.size());
    for (const auto& [genre, count] : genreCounts) {
        topGenres.push_back({ genre, count });
    }
    std::sort(topGenres.begin(), topGenres.end(), [](const auto& a, const auto& b) {
        if (a.second != b.second) return a.second > b.second;
        return ToLowerCopy(a.first) < ToLowerCopy(b.first);
    });

    std::string topGenresText;
    if (topGenres.empty()) {
        topGenresText = "No genres yet.";
    }
    else {
        const size_t topN = std::min<size_t>(3, topGenres.size());
        for (size_t i = 0; i < topN; ++i) {
            topGenresText += std::format("{}. {} ({})", i + 1, topGenres[i].first, topGenres[i].second);
            if (i + 1 < topN) topGenresText += "\n";
        }
    }

    m_AnalyticsSummaryLabel->SetText(std::format("Total books: {}", totalBooks));
    m_AnalyticsRatingLabel->SetText(std::format("Average rating: {:.2f}  |  Rated books: {}", avgRating, ratedCount));
    m_AnalyticsStatusLabel->SetText(std::format("Status split\nTo Read: {}    Reading: {}    Read: {}", toReadCount, readingCount, readCount));
    m_AnalyticsTimeLabel->SetText(std::format("Finished\nThis month: {}    This year: {}", finishedThisMonth, finishedThisYear));
    m_AnalyticsGenreLabel->SetText("Top genres\n" + topGenresText);
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





void UIManager::UpdateTooltipCache(const GraphManager* graphRenderer, const BookManager& bookManager, TextRenderer* textRenderer)
{
    if (m_LastHoveredNodeId == -1 || m_CachedTooltipText.empty() == false || textRenderer == nullptr) return;

    if (m_LastHoveredNodeType == NodeType::Book) {
        const Book* b = bookManager.findBookById(m_LastHoveredNodeId);
        if (b) {
            
            std::string genres = "";
            for (size_t i = 0; i < b->getGenres().size(); ++i) {
                genres += b->getGenres()[i] + (i < b->getGenres().size() - 1 ? ", " : "");
            }

            m_CachedTooltipText = "Title: " + b->getTitle() +
                "\nAuthor: " + b->getAuthor() +
                "\nStatus: " + statusToString(b->getStatus()) +
                "\nRating: " + Book::ratingToStars(b->getRating()) +
                "\nGenres: " + genres +
                "\nNotes: " + b->getNotes() +
                "\n(ID: " + std::to_string(b->getId()) + ")";
        }
    }
    else if (m_LastHoveredNodeType == NodeType::Genre) {
        m_CachedTooltipText = "Genre: " + graphRenderer->getGenreNameByNodeId(m_LastHoveredNodeId) +
            "\nBooks: " + std::to_string(graphRenderer->getNumOfConnectedBooks(m_LastHoveredNodeId)) +
            "\n(ID: " + std::to_string(m_LastHoveredNodeId) + ")";
    }

    const float fontSize = 20.0f;
    const int padding = 16;
    const int lineSpacing = 12;

    // Precompute wrapped lines and box metrics once to avoid doing it every Draw() call.
    m_CachedLines.clear();
    size_t start = 0, end;
    while ((end = m_CachedTooltipText.find('\n', start)) != std::string::npos) {
        m_CachedLines.push_back(m_CachedTooltipText.substr(start, end - start));
        start = end + 1;
    }
    m_CachedLines.push_back(m_CachedTooltipText.substr(start));

    float maxLineWidth = 0;
    for (const std::string& line : m_CachedLines) {
        float width = textRenderer->Measure(line, fontSize);
        if (width > maxLineWidth) maxLineWidth = width;
    }

    m_CachedBoxWidth = (int)maxLineWidth + 2 * padding;
    m_CachedBoxHeight = (int)((fontSize + lineSpacing) * m_CachedLines.size()) - lineSpacing + 2 * padding;
}

