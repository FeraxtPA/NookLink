#include "uiManager.h"
#include "UI/button.h"
#include "UI/textInput.h" 
#include "UI/panel.h"    
#include "UI/textBox.h"
#include "colors.h"
#include <format> 
#include "UI/widget.h"
#include "UI/flexLayout.h"
#include "logging.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <regex>
#include <sstream>

namespace {

std::string TrimCopy(const std::string& input)
{
    auto begin = std::find_if_not(input.begin(), input.end(), [](unsigned char c) { return std::isspace(c); });
    auto end = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char c) { return std::isspace(c); }).base();

    if (begin >= end) {
        return "";
    }

    return std::string(begin, end);
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

void AutoFillDatesForStatus(Status status, const std::shared_ptr<TextInput>& startedInput, const std::shared_ptr<TextInput>& finishedInput)
{
    if (!startedInput || !finishedInput) {
        return;
    }

    const std::string today = GetTodayDateDDMMYYYY();

    if ((status == Status::Reading || status == Status::Read) && startedInput->text.empty()) {
        startedInput->text = today;
    }

    if (status == Status::Read && finishedInput->text.empty()) {
        finishedInput->text = today;
    }
}

namespace UiMetrics {
constexpr float kToolbarShellMargin = 8.0f;
constexpr float kToolbarShellHeight = 98.0f;
constexpr float kToolbarRowTop = 54.0f;
constexpr float kToolbarRowHeight = 40.0f;
constexpr float kToolbarGap = 12.0f;
constexpr float kToolbarWidthInset = 52.0f;

constexpr float kToolbarBackWidth = 100.0f;
constexpr float kToolbarLoadWidth = 100.0f;
constexpr float kToolbarSaveWidth = 100.0f;
constexpr float kToolbarSaveAsWidth = 112.0f;
constexpr float kToolbarSearchWidth = 360.0f;
constexpr float kToolbarFiltersWidth = 104.0f;
constexpr float kToolbarUndoWidth = 90.0f;
constexpr float kToolbarRedoWidth = 90.0f;
constexpr float kToolbarAddBookWidth = 110.0f;
constexpr float kToolbarNextReadWidth = 120.0f;
constexpr float kToolbarToggleLayoutWidth = 148.0f;
constexpr float kToolbarGoalsWidth = 88.0f;

constexpr float kToolbarLabelY = 18.0f;
constexpr float kToolbarLibraryX = 24.0f;
constexpr float kToolbarSearchHalfLabelWidth = 35.0f;
constexpr float kToolbarActionsInset = 172.0f;
constexpr int kToolbarLabelFontSize = 14;

constexpr float kPanelTitleBarHeight = 40.0f;
constexpr float kPanelGap = 12.0f;
constexpr float kPanelButtonRowHeight = 40.0f;
constexpr float kPanelInputHeight = 35.0f;
constexpr float kPanelWideFieldWidth = 360.0f;
constexpr float kPanelButtonRowWidth = 364.0f;

constexpr Vector2 kPanelPaddingCompact = { 18.0f, 16.0f };
constexpr Vector2 kPanelPaddingDefault = { 18.0f, 18.0f };
constexpr Vector2 kPanelPaddingDetails = { 16.0f, 16.0f };

constexpr Vector2 kFilterPanelSize = { 300.0f, 500.0f };
constexpr Vector2 kFilterPanelOffset = { -175.0f, 0.0f };
constexpr float kFilterControlWidth = 250.0f;
constexpr float kFilterCheckboxSize = 20.0f;

constexpr Vector2 kDetailsPanelSize = { 320.0f, 500.0f };
constexpr Vector2 kDetailsPanelOffset = { 180.0f, 0.0f };
constexpr float kDetailsTextWidth = 280.0f;
constexpr float kDetailsTextHeight = 320.0f;
constexpr float kDetailsButtonRowWidth = 288.0f;

constexpr Vector2 kEditorPanelSize = { 400.0f, 660.0f };
constexpr Vector2 kEditorPanelOffset = { 0.0f, 0.0f };

constexpr Vector2 kLotteryPanelSize = { 400.0f, 350.0f };
constexpr Vector2 kLotteryPanelOffset = { 0.0f, 25.0f };
constexpr float kLotteryTextHeight = 180.0f;

constexpr Vector2 kGoalPanelSize = { 380.0f, 360.0f };
constexpr Vector2 kGoalPanelOffset = { 0.0f, 0.0f };
constexpr float kGoalButtonWidth = 80.0f;

constexpr float kHelpX = 10.0f;
constexpr float kHelpStartY = 136.0f;
constexpr float kHelpLineStep = 25.0f;

constexpr int kDefaultFontSize = 20;
constexpr float kNotificationMargin = 20.0f;
constexpr float kNotificationHeight = 50.0f;
constexpr float kNotificationTextPaddingX = 20.0f;
constexpr float kNotificationTextPaddingY = 15.0f;

constexpr float kTooltipOffset = 10.0f;
constexpr float kTooltipTextInset = 26.0f;
constexpr float kTooltipLineSpacing = 32.0f;
}

} // namespace

UIManager::UIManager(int screenWidth, int screenHeight)
    : m_ScreenWidth(screenWidth), m_ScreenHeight(screenHeight)
{}

UIManager::~UIManager() {}

void UIManager::OpenCalendarFor(const std::shared_ptr<TextInput>& targetInput)
{
    if (!targetInput || !m_CalendarWidget) {
        return;
    }

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

    // P��prava ��nr� (slou�en� do stringu)
    std::string genreStr = "Genres: ";
    const auto& genres = book->getGenres();
    for (size_t i = 0; i < genres.size(); ++i) {
        genreStr += genres[i];
        if (i < genres.size() - 1) genreStr += ", ";
    }

    // Slep�me v�echno do jednoho obrovsk�ho textu!
    // D�ky \n\n bude mezi ka�dou polo�kou hezk� pr�zdn� ��dek.
    std::string fullText =
        "Title: " + book->getTitle() + "\n\n" +
        "Author: " + book->getAuthor() + "\n\n" +
        std::format("Rating: {:.1f} / 5.0\n\n", book->getRating()) +
        "Added: " + (book->getDateAdded().empty() ? std::string("-") : book->getDateAdded()) + "\n\n" +
        "Started: " + (book->getDateStartedReading().empty() ? std::string("-") : book->getDateStartedReading()) + "\n\n" +
        "Finished: " + (book->getDateFinishedReading().empty() ? std::string("-") : book->getDateFinishedReading()) + "\n\n" +
        genreStr + "\n\n" +
        "Notes:\n" + book->getNotes();

    // Po�leme to na�emu chytr�mu word-wrap Labelu
    m_DetailsText->SetText(fullText);

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

    //Notification timer
    if (m_NotificationTimer > 0.0f) {
        m_NotificationTimer -= GetFrameTime();
    }

    // 1. Graph Interaction
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

    //Seach bar functionality
    if (graphRenderer) {
        std::string finalQuery = m_SearchBar->GetText(); // Text z horn� li�ty

        if (!m_ActiveFilterQuery.empty()) {
            if (!finalQuery.empty()) finalQuery += " | "; // Odd�lova�, pokud je vypln�no oboj�
            finalQuery += m_ActiveFilterQuery;            // P�id�me filtry ze slideru a ��nru
        }

        graphRenderer->setSearchQuery(finalQuery);
    }

    UpdateGoalPanelTexts();


    
    // lottery
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

                // maybe useless since it doesnt do anything since isrolling is set to false earilier
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
    
    // Handling after lotterry is done animating
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

    
    bool hoverBeforeUpdate = IsMouseOverUI();

    // Tady prob�h� tv�j norm�ln� update widget�
    for (auto it = m_Widgets.rbegin(); it != m_Widgets.rend(); ++it) {
        (*it)->Update();
    }

    if (m_CalendarWidget && !m_CalendarWidget->IsOpen()) {
        m_ActiveDateInput.reset();
    }

    // 2. Graf blokujeme, pokud jsme nad UI byli p�ed updatem, nebo jsme nad n�m po updatu
    isBlockingGraph = hoverBeforeUpdate || IsMouseOverUI();

    // 3. Vym�� tady ta star� vol�n� IsMouseOverUI() za na�i novou prom�nnou
    if (!isBlockingGraph) {
        UpdateTooltipCache(graphRenderer, bookManager, textRenderer);
        SetMouseCursor(Widget::DesiredCursor);
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

void UIManager::Draw(Vector2 mousePos, GraphManager* graphRenderer, const BookManager& bookManager, TextRenderer* textRenderer) const
{
    if (!textRenderer) return;

    // Top toolbar backdrop
    const Rectangle toolbarRect = {
        UiMetrics::kToolbarShellMargin,
        UiMetrics::kToolbarShellMargin,
        (float)m_ScreenWidth - (UiMetrics::kToolbarShellMargin * 2.0f),
        UiMetrics::kToolbarShellHeight
    };
    DrawRectangleRounded(toolbarRect, 0.16f, 16, Fade(NookCol::UI_SHELL, 0.98f));
    DrawRectangleRoundedLinesEx(toolbarRect, 0.16f, 16, 2.0f, Fade(NookCol::UI_BORDER_SOFT, 0.50f));
    DrawRectangleRounded({ toolbarRect.x + 1.0f, toolbarRect.y + 1.0f, toolbarRect.width - 2.0f, 28.0f }, 0.12f, 16, Fade(WHITE, 0.04f));

    // Section labels
    textRenderer->DrawSimpleText("Library", { UiMetrics::kToolbarLibraryX, UiMetrics::kToolbarLabelY }, UiMetrics::kToolbarLabelFontSize, Fade(NookCol::UI_TEXT, 0.90f));
    textRenderer->DrawSimpleText("Search", { m_ScreenWidth / 2.0f - UiMetrics::kToolbarSearchHalfLabelWidth, UiMetrics::kToolbarLabelY }, UiMetrics::kToolbarLabelFontSize, Fade(NookCol::UI_TEXT, 0.90f));
    textRenderer->DrawSimpleText("Actions", { (float)m_ScreenWidth - UiMetrics::kToolbarActionsInset, UiMetrics::kToolbarLabelY }, UiMetrics::kToolbarLabelFontSize, Fade(NookCol::UI_TEXT, 0.90f));

    DrawHelpText(textRenderer);
    textRenderer->DrawSimpleText(std::to_string(GetFPS()), { 10, (float)m_ScreenHeight - 20 }, 20, NookCol::UI_ACCENT_SOFT);

    for (auto& w : m_Widgets) w->Draw(textRenderer);

    if (graphRenderer != nullptr) {
        std::string count = "Nodes: " + std::to_string(graphRenderer->getNodes().size());
        textRenderer->DrawSimpleText(count, { 10, (float)m_ScreenHeight - 40 }, 20, NookCol::UI_TEXT_MUTED);
    }

    if (!IsMouseOverUI()) {
        DrawTooltip(mousePos, textRenderer);
    }

    DrawNotification(textRenderer);
}



void UIManager::DrawHelpText(TextRenderer* renderer) const
{
    renderer->DrawSimpleText("Right-click drag: Move | Space: Add Books", { UiMetrics::kHelpX, UiMetrics::kHelpStartY + (UiMetrics::kHelpLineStep * 0.0f) }, UiMetrics::kDefaultFontSize, NookCol::UI_TEXT_MUTED);
    renderer->DrawSimpleText("Shift+Drag: Lock | Shift+Click: Delete", { UiMetrics::kHelpX, UiMetrics::kHelpStartY + (UiMetrics::kHelpLineStep * 1.0f) }, UiMetrics::kDefaultFontSize, NookCol::UI_TEXT_MUTED);
    renderer->DrawSimpleText("Middle Click: Pan | Scroll: Zoom", { UiMetrics::kHelpX, UiMetrics::kHelpStartY + (UiMetrics::kHelpLineStep * 2.0f) }, UiMetrics::kDefaultFontSize, NookCol::UI_TEXT_MUTED);
    renderer->DrawSimpleText("Double Click: Unlock Node | 'E': Edit Node", { UiMetrics::kHelpX, UiMetrics::kHelpStartY + (UiMetrics::kHelpLineStep * 3.0f) }, UiMetrics::kDefaultFontSize, NookCol::UI_TEXT_MUTED);
    renderer->DrawSimpleText("Ctrl+Z: Undo | Ctrl+Y: Redo", { UiMetrics::kHelpX, UiMetrics::kHelpStartY + (UiMetrics::kHelpLineStep * 4.0f) }, UiMetrics::kDefaultFontSize, NookCol::UI_TEXT_MUTED);
    renderer->DrawSimpleText("V: Unlock FPS | B: Enable VSync", { UiMetrics::kHelpX, UiMetrics::kHelpStartY + (UiMetrics::kHelpLineStep * 5.0f) }, UiMetrics::kDefaultFontSize, NookCol::UI_TEXT_MUTED);
}

void UIManager::DrawTooltip(Vector2 mousePos, TextRenderer* renderer) const
{
    if (m_LastHoveredNodeId != -1 && (GetTime() - m_HoverStartTime >= 0.5) && !IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        DrawRectangleRounded({ mousePos.x + UiMetrics::kTooltipOffset, mousePos.y + UiMetrics::kTooltipOffset, (float)m_CachedBoxWidth + 20, (float)m_CachedBoxHeight + 20 }, 0.2f, 10, Fade(NookCol::POPUP_BORDER, 0.75f));
        DrawRectangleRounded({ mousePos.x + UiMetrics::kTooltipOffset, mousePos.y + UiMetrics::kTooltipOffset, (float)m_CachedBoxWidth + 16, (float)m_CachedBoxHeight + 16 }, 0.2f, 10, Fade(NookCol::POPUP_BG, 0.95f));

        float xStart = mousePos.x + UiMetrics::kTooltipTextInset;
        float yStart = mousePos.y + UiMetrics::kTooltipTextInset;
        float spacing = UiMetrics::kTooltipLineSpacing;

        for (const auto& line : m_CachedLines) {
            renderer->DrawSimpleText(line, { xStart, yStart }, 20, NookCol::TEXT_DEFAULT);
            yStart += spacing;
        }
    }
}

void UIManager::ShowNotification(const std::string& message, float duration)
{
    m_NotificationText = message;
    m_NotificationTimer = duration;
}

void UIManager::DrawNotification(TextRenderer* textRenderer) const
{
    if (m_NotificationTimer <= 0.0f || !textRenderer) return;

    // Vypo��t�me pr�hlednost (Alpha). 
    // Pokud zb�v� m�n� ne� 0.5 sekundy, za�ne plynule mizet.
    float alpha = 1.0f;
    if (m_NotificationTimer < 0.5f) {
        alpha = m_NotificationTimer / 0.5f;
    }

    // Nastaven� rozm�r�
    int fontSize = UiMetrics::kDefaultFontSize;
    float textWidth = textRenderer->Measure(m_NotificationText, fontSize);
    float boxWidth = textWidth + (UiMetrics::kNotificationTextPaddingX * 2.0f);
    float boxHeight = UiMetrics::kNotificationHeight;

    // Pozice: Prav� doln� roh
    float x = m_ScreenWidth - boxWidth - UiMetrics::kNotificationMargin;
    float y = m_ScreenHeight - boxHeight - UiMetrics::kNotificationMargin;

    // Barvy s aplikovanou pr�hlednost� (Fade)
    Color boxColor = Fade(NookCol::UI_SHELL, alpha * 0.96f);
    Color textColor = Fade(NookCol::UI_TEXT, alpha);

    // Vykreslen� pozad� a textu
    DrawRectangleRounded({ x, y, boxWidth, boxHeight }, 0.3f, 10, boxColor);
    textRenderer->DrawSimpleText(m_NotificationText, { x + UiMetrics::kNotificationTextPaddingX, y + UiMetrics::kNotificationTextPaddingY }, fontSize, textColor);
}

bool UIManager::IsMouseOverUI() const {
    const Vector2 mouse = GetMousePosition();
    const Rectangle toolbarRect = {
        UiMetrics::kToolbarShellMargin,
        UiMetrics::kToolbarShellMargin,
        (float)m_ScreenWidth - (UiMetrics::kToolbarShellMargin * 2.0f),
        UiMetrics::kToolbarShellHeight
    };

    if (CheckCollisionPointRec(mouse, toolbarRect)) {
        return true;
    }

    for (const auto& w : m_Widgets) {
        if (!w->isVisible) {
            continue;
        }

        if (w->isHovered || CheckCollisionPointRec(mouse, w->m_Bounds)) {
            return true;
        }
    }
    return false;
}

void UIManager::BuildInterface(
    std::function<void()> onSave,
    std::function<void()> onSaveAs,
    std::function<void()> onLoad,
    std::function<void()> onBackToMenu,
    std::function<void()> onUndo,
    std::function<void()> onRedo,
    std::function<void(std::string, std::string, std::string, float, Status, std::string, std::string, std::string)> onAddBook,
    std::function<void(int, std::string, std::string, std::string, float, Status, std::string, std::string, std::string)> onEditBook,
    std::function<void()> onToggleLayout,
    std::function<void(Status)> onToggleStatus,
    std::function<int()> getReadCount,
    std::function<int()> getGoalTarget,
    std::function<int()> getGoalProgress,
    std::function<void(int)> onAdjustGoalTarget,
    std::function<void()> onResetGoalProgress,
    std::function<void(int)> onSortBooks)
{

    m_Widgets.clear();

    m_GetReadCount = std::move(getReadCount);
    m_GetGoalTarget = std::move(getGoalTarget);
    m_GetGoalProgress = std::move(getGoalProgress);
    m_OnAdjustGoalTarget = std::move(onAdjustGoalTarget);
    m_OnResetGoalProgress = std::move(onResetGoalProgress);

    const float toolbarRowTop = UiMetrics::kToolbarRowTop;
    const float toolbarRowHeight = UiMetrics::kToolbarRowHeight;
    const float toolbarGap = UiMetrics::kToolbarGap;

    auto makeToolbarButton = [toolbarRowHeight](const std::string& label, std::function<void()> callback, float width) {
        return std::make_shared<Button>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ width, toolbarRowHeight }, label, std::move(callback));
    };

    auto makeButtonRow = [](Vector2 size, float gap = UiMetrics::kPanelGap) {
        return std::make_shared<FlexLayout>(
            Anchor::TopLeft,
            Vector2{ 0.0f, 0.0f },
            size,
            FlexLayout::Direction::Horizontal,
            Vector2{ 0.0f, 0.0f },
            gap,
            FlexLayout::CrossAlign::Stretch
        );
    };

    auto makeDateInputRow = [this, makeButtonRow](const std::shared_ptr<TextInput>& input) {
        auto row = makeButtonRow({ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight }, 8.0f);

        auto pickButton = std::make_shared<Button>(
            Anchor::TopLeft,
            Vector2{ 0.0f, 0.0f },
            Vector2{ 68.0f, UiMetrics::kPanelInputHeight },
            "Pick",
            [this, input]() {
                OpenCalendarFor(input);
            }
        );

        row->AddChild(input, { UiMetrics::kPanelWideFieldWidth - 68.0f - 8.0f, UiMetrics::kPanelInputHeight }, 1.0f);
        row->AddChild(pickButton, { 68.0f, UiMetrics::kPanelInputHeight });
        return row;
    };

    // === 1. VYTVO�EN� FILTER PANELU ===
    // Um�st�me ho nap�. vlevo nahoru pod vyhled�v�n� (nastav offsety dle sv�ho UI)
    m_SearchFilterPanel = std::make_shared<Panel>(Anchor::CenterRight, UiMetrics::kFilterPanelOffset, UiMetrics::kFilterPanelSize,"Search Filter");
    m_SearchFilterPanel->isVisible = false; // V�choz� stav: schovan�

    auto filterLayout = m_SearchFilterPanel->CreateContentLayout(
        FlexLayout::Direction::Vertical,
        UiMetrics::kPanelPaddingCompact,
        UiMetrics::kPanelGap,
        FlexLayout::CrossAlign::Start
    );

    // V�choz� stav uzl� je viditeln�, proto d�v�me "initial = true"
    m_CheckToRead = std::make_shared<Checkbox>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kFilterCheckboxSize, UiMetrics::kFilterCheckboxSize }, "To Read", true,
        [onToggleStatus](bool checked) { onToggleStatus(Status::ToRead); }
    );

    m_CheckReading = std::make_shared<Checkbox>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kFilterCheckboxSize, UiMetrics::kFilterCheckboxSize }, "Reading", true,
        [onToggleStatus](bool checked) { onToggleStatus(Status::Reading); }
    );

    m_CheckRead = std::make_shared<Checkbox>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kFilterCheckboxSize, UiMetrics::kFilterCheckboxSize }, "Read", true,
        [onToggleStatus](bool checked) { onToggleStatus(Status::Read);}
    );
    m_FilterRatingLabel = std::make_shared<Label>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kFilterControlWidth, 30.0f }, "Min. Rating: 2.5");
    
    m_FilterRatingSlider = std::make_shared<Slider>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kFilterControlWidth, 30.0f }, 0.0f, 5.0f, 2.5f,
        [this](float val) {
            m_FilterRatingLabel->SetText(std::format("Min. Hodnoceni: {:.1f}", val));
        }
    );
    m_FilterGenreLabel = std::make_shared<Label>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kFilterControlWidth, 30.0f }, "Genre Filter");
    
    m_FilterGenreInput = std::make_shared<TextInput>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kFilterControlWidth, 30.0f }, "");

    m_FilterFinishedRangeState = 0;
    m_FilterFinishedRangeBtn = std::make_shared<Button>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ UiMetrics::kFilterControlWidth, UiMetrics::kPanelButtonRowHeight },
        "Finished: All",
        [this]() {
            m_FilterFinishedRangeState = (m_FilterFinishedRangeState + 1) % 3;
            if (m_FilterFinishedRangeState == 0) m_FilterFinishedRangeBtn->SetText("Finished: All");
            else if (m_FilterFinishedRangeState == 1) m_FilterFinishedRangeBtn->SetText("Finished: This Month");
            else m_FilterFinishedRangeBtn->SetText("Finished: This Year");
        }
    );

    m_FilterSortState = 0;
    m_FilterSortBtn = std::make_shared<Button>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ UiMetrics::kFilterControlWidth, UiMetrics::kPanelButtonRowHeight },
        "Sort: ID",
        [this, onSortBooks]() {
            m_FilterSortState = (m_FilterSortState + 1) % 4;
            if (m_FilterSortState == 0) m_FilterSortBtn->SetText("Sort: ID");
            else if (m_FilterSortState == 1) m_FilterSortBtn->SetText("Sort: Author");
            else if (m_FilterSortState == 2) m_FilterSortBtn->SetText("Sort: Rating");
            else m_FilterSortBtn->SetText("Sort: Added Date");

            if (onSortBooks) {
                onSortBooks(m_FilterSortState);
            }
        }
    );

    m_ApplyFiltersBtn = std::make_shared<Button>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kFilterControlWidth, UiMetrics::kPanelButtonRowHeight }, "Apply Filters",
        [this]() { 
            std::string query = "";

            float rating = m_FilterRatingSlider->value;
            if (rating > 0.0f) {
                query += "r>" + std::format("{:.1f}", rating);
            }

            if (!m_FilterGenreInput->text.empty()) {
                if (!query.empty()) query += " | ";
                query += "g:" + m_FilterGenreInput->text;
            }

            if (m_FilterFinishedRangeState == 1) {
                if (!query.empty()) query += " | ";
                query += "fr:month";
            }
            else if (m_FilterFinishedRangeState == 2) {
                if (!query.empty()) query += " | ";
                query += "fr:year";
            }

            // Ulo��me filtry, metoda Update() u� se postar� o zbytek!
            m_ActiveFilterQuery = query;
        }
    );

    filterLayout->AddChild(m_CheckToRead, { UiMetrics::kFilterCheckboxSize, UiMetrics::kFilterCheckboxSize });
    filterLayout->AddChild(m_CheckReading, { UiMetrics::kFilterCheckboxSize, UiMetrics::kFilterCheckboxSize });
    filterLayout->AddChild(m_CheckRead, { UiMetrics::kFilterCheckboxSize, UiMetrics::kFilterCheckboxSize });
    filterLayout->AddChild(m_FilterRatingLabel, { UiMetrics::kFilterControlWidth, 30.0f });
    filterLayout->AddChild(m_FilterRatingSlider, { UiMetrics::kFilterControlWidth, 30.0f });
    filterLayout->AddChild(m_FilterGenreLabel, { UiMetrics::kFilterControlWidth, 30.0f });
    filterLayout->AddChild(m_FilterGenreInput, { UiMetrics::kFilterControlWidth, 30.0f });
    filterLayout->AddChild(m_FilterFinishedRangeBtn, { UiMetrics::kFilterControlWidth, UiMetrics::kPanelButtonRowHeight });
    filterLayout->AddChild(m_FilterSortBtn, { UiMetrics::kFilterControlWidth, UiMetrics::kPanelButtonRowHeight });
    filterLayout->AddSpacer(1.0f);
    filterLayout->AddChild(m_ApplyFiltersBtn, { UiMetrics::kFilterControlWidth, UiMetrics::kPanelButtonRowHeight });

    // P�id�me panel do hlavn�ho seznamu widget� (z�le��, jak vykresluje� panely, 
    // pravd�podobn� d�l� m_Widgets.push_back)
    m_Widgets.push_back(m_SearchFilterPanel);

    // ============================================================
    // 1. HLAVN� UI - horn� toolbar
    // ============================================================
    m_ToolbarLayout = std::make_shared<FlexLayout>(
        Anchor::TopCenter,
        Vector2{ 0.0f, toolbarRowTop + (toolbarRowHeight / 2.0f) },
        Vector2{ std::max(0.0f, (float)m_ScreenWidth - UiMetrics::kToolbarWidthInset), toolbarRowHeight },
        FlexLayout::Direction::Horizontal,
        Vector2{ 0.0f, 0.0f },
        toolbarGap,
        FlexLayout::CrossAlign::Center
    );

    m_ToolbarLayout->AddChild(makeToolbarButton("Back", onBackToMenu, UiMetrics::kToolbarBackWidth), { UiMetrics::kToolbarBackWidth, toolbarRowHeight });
    m_ToolbarLayout->AddChild(makeToolbarButton("Load", onLoad, UiMetrics::kToolbarLoadWidth), { UiMetrics::kToolbarLoadWidth, toolbarRowHeight });
    m_ToolbarLayout->AddChild(makeToolbarButton("Save", onSave, UiMetrics::kToolbarSaveWidth), { UiMetrics::kToolbarSaveWidth, toolbarRowHeight });
    m_ToolbarLayout->AddChild(makeToolbarButton("Save As", onSaveAs, UiMetrics::kToolbarSaveAsWidth), { UiMetrics::kToolbarSaveAsWidth, toolbarRowHeight });

    m_ToolbarLayout->AddSpacer(1.0f);

    m_SearchBar = std::make_shared<TextInput>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kToolbarSearchWidth, toolbarRowHeight }, "Search Books/Authors...");
    m_ToolbarLayout->AddChild(m_SearchBar, { UiMetrics::kToolbarSearchWidth, toolbarRowHeight });

    m_ToolbarLayout->AddChild(makeToolbarButton("Filters",
        [this]() {
            static double lastClickTime = 0;
            if (GetTime() - lastClickTime > 0.2) {
                m_SearchFilterPanel->isVisible = !m_SearchFilterPanel->isVisible;
                lastClickTime = GetTime();
            }
        },
        UiMetrics::kToolbarFiltersWidth
    ), { UiMetrics::kToolbarFiltersWidth, toolbarRowHeight });

    m_ToolbarLayout->AddChild(makeToolbarButton("Goals",
        [this]() {
            if (m_ReadingGoalPanel) {
                m_ReadingGoalPanel->isVisible = !m_ReadingGoalPanel->isVisible;
            }
        },
        UiMetrics::kToolbarGoalsWidth
    ), { UiMetrics::kToolbarGoalsWidth, toolbarRowHeight });

    m_ToolbarLayout->AddSpacer(1.0f);

    m_ToolbarLayout->AddChild(makeToolbarButton("Undo", onUndo, UiMetrics::kToolbarUndoWidth), { UiMetrics::kToolbarUndoWidth, toolbarRowHeight });
    m_ToolbarLayout->AddChild(makeToolbarButton("Redo", onRedo, UiMetrics::kToolbarRedoWidth), { UiMetrics::kToolbarRedoWidth, toolbarRowHeight });
    m_ToolbarLayout->AddChild(makeToolbarButton("Add Book",
        [this]() { m_AddPanel->isVisible = true; },
        UiMetrics::kToolbarAddBookWidth
    ), { UiMetrics::kToolbarAddBookWidth, toolbarRowHeight });
    m_ToolbarLayout->AddChild(makeToolbarButton("Next Read",
        [this]() {
            m_LotteryPanel->isVisible = true; m_IsLotteryRolling = true;
            m_LotteryTimer = 2.0f; m_LotterySpeedTimer = 0.0f;
            m_LotteryCloseBtn->isVisible = false; m_LotteryText->SetText("Spinning...");
        },
        UiMetrics::kToolbarNextReadWidth
    ), { UiMetrics::kToolbarNextReadWidth, toolbarRowHeight });
    m_ToolbarLayout->AddChild(makeToolbarButton("Toggle Layout", onToggleLayout, UiMetrics::kToolbarToggleLayoutWidth), { UiMetrics::kToolbarToggleLayoutWidth, toolbarRowHeight });

    m_Widgets.push_back(m_ToolbarLayout);

    
    m_BookDetailsPanel = std::make_shared<Panel>(Anchor::CenterLeft, UiMetrics::kDetailsPanelOffset, UiMetrics::kDetailsPanelSize, "Book Info");
    m_BookDetailsPanel->isVisible = false;

    auto detailsLayout = m_BookDetailsPanel->CreateContentLayout(
        FlexLayout::Direction::Vertical,
        UiMetrics::kPanelPaddingDetails,
        UiMetrics::kPanelGap,
        FlexLayout::CrossAlign::Start
    );

    // Prvky panelu
    m_DetailsText = std::make_shared<Label>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kDetailsTextWidth, UiMetrics::kDetailsTextHeight }, "", 20, NookCol::UI_TEXT, true);

    m_DetailsEditBtn = std::make_shared<Button>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ 120.0f, UiMetrics::kPanelButtonRowHeight }, "Edit Book",
        [this]() {
            if (m_HasCurrentDetailsBook) {
                OpenEditPanel(&m_CurrentDetailsBook);
                m_BookDetailsPanel->isVisible = false;
            }
        }
    );
    m_DetailsCloseBtn = std::make_shared<Button>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ 120.0f, UiMetrics::kPanelButtonRowHeight }, "Close",
        [this]() { m_BookDetailsPanel->isVisible = false; }
    );

    auto detailsButtonRow = makeButtonRow({ UiMetrics::kDetailsButtonRowWidth, UiMetrics::kPanelButtonRowHeight });
    detailsButtonRow->AddChild(m_DetailsEditBtn, { 0.0f, UiMetrics::kPanelButtonRowHeight }, 1.0f);
    detailsButtonRow->AddChild(m_DetailsCloseBtn, { 0.0f, UiMetrics::kPanelButtonRowHeight }, 1.0f);

    detailsLayout->AddChild(m_DetailsText, { UiMetrics::kDetailsTextWidth, UiMetrics::kDetailsTextHeight });
    detailsLayout->AddSpacer(1.0f);
    detailsLayout->AddChild(detailsButtonRow, { UiMetrics::kDetailsButtonRowWidth, UiMetrics::kPanelButtonRowHeight });

    m_Widgets.push_back(m_BookDetailsPanel);

    // ============================================================
    // 2. ADD BOOK PANEL (Uprost�ed)
    // ============================================================
    m_AddPanel = std::make_shared<Panel>(Anchor::Center, UiMetrics::kEditorPanelOffset, UiMetrics::kEditorPanelSize, "Add New Book");
    m_AddPanel->isVisible = false;

    auto addLayout = m_AddPanel->CreateContentLayout(
        FlexLayout::Direction::Vertical,
        UiMetrics::kPanelPaddingDefault,
        UiMetrics::kPanelGap,
        FlexLayout::CrossAlign::Start
    );

    m_AddTitle = std::make_shared<TextInput>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight }, "Title");
    m_AddAuthor = std::make_shared<TextInput>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight }, "Author");
    m_AddGenres = std::make_shared<TextInput>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight }, "Genres (e.g. SciFi, Horror)");
    m_AddRating = std::make_shared<TextInput>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight }, "Rating (0.0 - 5.0)");
    m_AddStartedDate = std::make_shared<TextInput>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight }, "Started Reading (DD.MM.YYYY)");
    m_AddFinishedDate = std::make_shared<TextInput>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight }, "Finished Reading (DD.MM.YYYY)");
    auto addStartedRow = makeDateInputRow(m_AddStartedDate);
    auto addFinishedRow = makeDateInputRow(m_AddFinishedDate);
    m_AddNotes = std::make_shared<TextBox>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, 100.0f }, "Notes");

    m_AddStatusState = std::make_shared<int>(0);
    m_AddStatusBtn = std::make_shared<Button>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelButtonRowHeight }, "Status: To Read", []() {});

    std::weak_ptr<Button> weakAddBtn = m_AddStatusBtn;
    m_AddStatusBtn->SetOnClick([this, weakAddBtn]() {
        if (auto btn = weakAddBtn.lock()) {
            *m_AddStatusState = (*m_AddStatusState + 1) % 3;
            if (*m_AddStatusState == 0) btn->SetText("Status: To Read");
            else if (*m_AddStatusState == 1) btn->SetText("Status: Reading");
            else btn->SetText("Status: Read");

            Status selected = Status::ToRead;
            if (*m_AddStatusState == 1) selected = Status::Reading;
            if (*m_AddStatusState == 2) selected = Status::Read;
            AutoFillDatesForStatus(selected, m_AddStartedDate, m_AddFinishedDate);
        }
    });

    auto btnCreate = std::make_shared<Button>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ 100.0f, UiMetrics::kPanelButtonRowHeight }, "Create",
        [this, onAddBook]() {
            const std::string t = TrimCopy(m_AddTitle->GetText());
            const std::string a = TrimCopy(m_AddAuthor->GetText());
            if (t.empty() || a.empty()) {
                ShowNotification("Title and Author are required.");
                Log::Warn("Add book blocked: missing Title or Author");
                return;
            }

            float r = 0.0f;
            if (!TryParseRating(m_AddRating->GetText(), r)) {
                ShowNotification("Rating must be a valid number.");
                Log::Warn("Add book blocked: invalid rating format");
                return;
            }

            if (r < 0.0f || r > 5.0f) {
                ShowNotification("Rating must be between 0.0 and 5.0.");
                Log::Warn("Add book blocked: rating out of range");
                return;
            }

            const std::string started = TrimCopy(m_AddStartedDate->GetText());
            const std::string finished = TrimCopy(m_AddFinishedDate->GetText());
            if (!IsValidDateDDMMYYYY(started) || !IsValidDateDDMMYYYY(finished)) {
                ShowNotification("Dates must be in DD.MM.YYYY format.");
                Log::Warn("Add book blocked: invalid date format");
                return;
            }

            Status s = Status::ToRead;
            if (*m_AddStatusState == 1) s = Status::Reading;
            if (*m_AddStatusState == 2) s = Status::Read;

            AutoFillDatesForStatus(s, m_AddStartedDate, m_AddFinishedDate);

            const std::string normalizedStarted = TrimCopy(m_AddStartedDate->GetText());
            const std::string normalizedFinished = TrimCopy(m_AddFinishedDate->GetText());
            if (!normalizedStarted.empty() && !normalizedFinished.empty() && CompareDateDDMMYYYY(normalizedStarted, normalizedFinished) > 0) {
                ShowNotification("Finished date cannot be earlier than started date.");
                Log::Warn("Add book blocked: finished date earlier than started date");
                return;
            }

            onAddBook(t, a, m_AddGenres->GetText(), r, s, m_AddNotes->GetText(), normalizedStarted, normalizedFinished);
            m_AddTitle->Clear(); m_AddAuthor->Clear(); m_AddGenres->Clear(); m_AddRating->Clear(); m_AddStartedDate->Clear(); m_AddFinishedDate->Clear(); m_AddNotes->Clear();
            if (m_CalendarWidget) m_CalendarWidget->Close();
            m_ActiveDateInput.reset();
            m_AddPanel->isVisible = false;
        }
    );

    auto btnCancelAdd = std::make_shared<Button>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ 100.0f, UiMetrics::kPanelButtonRowHeight }, "Cancel",
        [this]() {
            m_AddTitle->Clear(); m_AddAuthor->Clear(); m_AddGenres->Clear(); m_AddRating->Clear(); m_AddStartedDate->Clear(); m_AddFinishedDate->Clear(); m_AddNotes->Clear();
            if (m_CalendarWidget) m_CalendarWidget->Close();
            m_ActiveDateInput.reset();
            m_AddPanel->isVisible = false;
        }
    );

    auto addButtonRow = makeButtonRow({ UiMetrics::kPanelButtonRowWidth, UiMetrics::kPanelButtonRowHeight });
    addButtonRow->AddChild(btnCreate, { 0.0f, UiMetrics::kPanelButtonRowHeight }, 1.0f);
    addButtonRow->AddChild(btnCancelAdd, { 0.0f, UiMetrics::kPanelButtonRowHeight }, 1.0f);

    addLayout->AddChild(m_AddTitle, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    addLayout->AddChild(m_AddAuthor, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    addLayout->AddChild(m_AddGenres, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    addLayout->AddChild(m_AddRating, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    addLayout->AddChild(addStartedRow, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    addLayout->AddChild(addFinishedRow, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    addLayout->AddChild(m_AddNotes, { UiMetrics::kPanelWideFieldWidth, 100.0f });
    addLayout->AddChild(m_AddStatusBtn, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelButtonRowHeight });
    addLayout->AddSpacer(1.0f);
    addLayout->AddChild(addButtonRow, { UiMetrics::kPanelButtonRowWidth, UiMetrics::kPanelButtonRowHeight });

    // ============================================================
    // 3. EDIT BOOK PANEL (Uprost�ed)
    // ============================================================
    m_EditPanel = std::make_shared<Panel>(Anchor::Center, UiMetrics::kEditorPanelOffset, UiMetrics::kEditorPanelSize, "Edit Book Details");
    m_EditPanel->isVisible = false;

    auto editLayout = m_EditPanel->CreateContentLayout(
        FlexLayout::Direction::Vertical,
        UiMetrics::kPanelPaddingDefault,
        UiMetrics::kPanelGap,
        FlexLayout::CrossAlign::Start
    );

    m_EditTitle = std::make_shared<TextInput>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight }, "Title");
    m_EditAuthor = std::make_shared<TextInput>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight }, "Author");
    m_EditGenres = std::make_shared<TextInput>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight }, "Genres");
    m_EditRating = std::make_shared<TextInput>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight }, "Rating");
    m_EditStartedDate = std::make_shared<TextInput>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight }, "Started Reading (DD.MM.YYYY)");
    m_EditFinishedDate = std::make_shared<TextInput>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight }, "Finished Reading (DD.MM.YYYY)");
    auto editStartedRow = makeDateInputRow(m_EditStartedDate);
    auto editFinishedRow = makeDateInputRow(m_EditFinishedDate);
    m_EditNotes = std::make_shared<TextBox>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, 100.0f }, "Notes");

    m_EditStatusState = std::make_shared<int>(0);
    m_EditStatusBtn = std::make_shared<Button>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelButtonRowHeight }, "Status", []() {});

    std::weak_ptr<Button> weakEditBtn = m_EditStatusBtn;
    auto editState = m_EditStatusState;
    m_EditStatusBtn->SetOnClick([this, editState, weakEditBtn]() {
        if (auto btn = weakEditBtn.lock()) {
            *editState = (*editState + 1) % 3;
            if (*editState == 0) btn->SetText("Status: To Read");
            else if (*editState == 1) btn->SetText("Status: Reading");
            else btn->SetText("Status: Read");

            Status selected = Status::ToRead;
            if (*editState == 1) selected = Status::Reading;
            if (*editState == 2) selected = Status::Read;
            AutoFillDatesForStatus(selected, m_EditStartedDate, m_EditFinishedDate);
        }
        });

    auto btnUpdate = std::make_shared<Button>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ 100.0f, UiMetrics::kPanelButtonRowHeight }, "Update",
        [this, onEditBook, editState]() {
            const std::string title = TrimCopy(m_EditTitle->GetText());
            const std::string author = TrimCopy(m_EditAuthor->GetText());
            if (title.empty() || author.empty()) {
                ShowNotification("Title and Author are required.");
                Log::Warn("Edit book blocked: missing Title or Author");
                return;
            }

            float r = 0.0f;
            if (!TryParseRating(m_EditRating->GetText(), r)) {
                ShowNotification("Rating must be a valid number.");
                Log::Warn("Edit book blocked: invalid rating format");
                return;
            }

            if (r < 0.0f || r > 5.0f) {
                ShowNotification("Rating must be between 0.0 and 5.0.");
                Log::Warn("Edit book blocked: rating out of range");
                return;
            }

            const std::string started = TrimCopy(m_EditStartedDate->GetText());
            const std::string finished = TrimCopy(m_EditFinishedDate->GetText());
            if (!IsValidDateDDMMYYYY(started) || !IsValidDateDDMMYYYY(finished)) {
                ShowNotification("Dates must be in DD.MM.YYYY format.");
                Log::Warn("Edit book blocked: invalid date format");
                return;
            }

            Status s = Status::ToRead;
            if (*editState == 1) s = Status::Reading;
            if (*editState == 2) s = Status::Read;

            AutoFillDatesForStatus(s, m_EditStartedDate, m_EditFinishedDate);

            const std::string normalizedStarted = TrimCopy(m_EditStartedDate->GetText());
            const std::string normalizedFinished = TrimCopy(m_EditFinishedDate->GetText());
            if (!normalizedStarted.empty() && !normalizedFinished.empty() && CompareDateDDMMYYYY(normalizedStarted, normalizedFinished) > 0) {
                ShowNotification("Finished date cannot be earlier than started date.");
                Log::Warn("Edit book blocked: finished date earlier than started date");
                return;
            }

            onEditBook(m_EditingBookId, title, author, m_EditGenres->GetText(), r, s, m_EditNotes->GetText(), normalizedStarted, normalizedFinished);
            if (m_CalendarWidget) m_CalendarWidget->Close();
            m_ActiveDateInput.reset();
            m_EditPanel->isVisible = false;
        }
    );

    auto btnCancelEdit = std::make_shared<Button>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ 100.0f, UiMetrics::kPanelButtonRowHeight }, "Cancel",
        [this]() {
            if (m_CalendarWidget) m_CalendarWidget->Close();
            m_ActiveDateInput.reset();
            m_EditPanel->isVisible = false;
        }
    );

    auto editButtonRow = makeButtonRow({ UiMetrics::kPanelButtonRowWidth, UiMetrics::kPanelButtonRowHeight });
    editButtonRow->AddChild(btnUpdate, { 0.0f, UiMetrics::kPanelButtonRowHeight }, 1.0f);
    editButtonRow->AddChild(btnCancelEdit, { 0.0f, UiMetrics::kPanelButtonRowHeight }, 1.0f);

    editLayout->AddChild(m_EditTitle, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    editLayout->AddChild(m_EditAuthor, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    editLayout->AddChild(m_EditGenres, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    editLayout->AddChild(m_EditRating, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    editLayout->AddChild(editStartedRow, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    editLayout->AddChild(editFinishedRow, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    editLayout->AddChild(m_EditNotes, { UiMetrics::kPanelWideFieldWidth, 100.0f });
    editLayout->AddChild(m_EditStatusBtn, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelButtonRowHeight });
    editLayout->AddSpacer(1.0f);
    editLayout->AddChild(editButtonRow, { UiMetrics::kPanelButtonRowWidth, UiMetrics::kPanelButtonRowHeight });

    // ============================================================
    // 4. LOTTERY PANEL (Uprost�ed)
    // ============================================================
    m_LotteryPanel = std::make_shared<Panel>(Anchor::Center, UiMetrics::kLotteryPanelOffset, UiMetrics::kLotteryPanelSize, "Next Read Lottery");
    m_LotteryPanel->isVisible = false;

    auto lotteryLayout = m_LotteryPanel->CreateContentLayout(
        FlexLayout::Direction::Vertical,
        UiMetrics::kPanelPaddingCompact,
        UiMetrics::kPanelGap,
        FlexLayout::CrossAlign::Start
    );

    m_LotteryText = std::make_shared<TextBox>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kLotteryTextHeight }, "...");
    m_LotteryText->SetEditable(false);

    m_LotteryAutoRead = std::make_shared<Checkbox>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kFilterCheckboxSize, UiMetrics::kFilterCheckboxSize }, "Set status to 'Reading' automatically");

    m_LotteryCloseBtn = std::make_shared<Button>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ 100.0f, UiMetrics::kPanelButtonRowHeight }, "Close!",
        [this]() { m_LotteryPanel->isVisible = false; }
    );
    m_LotteryCloseBtn->isVisible = false;

    auto lotteryCloseRow = makeButtonRow({ UiMetrics::kPanelButtonRowWidth, UiMetrics::kPanelButtonRowHeight });
    lotteryCloseRow->AddSpacer(1.0f);
    lotteryCloseRow->AddChild(m_LotteryCloseBtn, { 100.0f, UiMetrics::kPanelButtonRowHeight });
    lotteryCloseRow->AddSpacer(1.0f);

    lotteryLayout->AddChild(m_LotteryText, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kLotteryTextHeight });
    lotteryLayout->AddChild(m_LotteryAutoRead, { UiMetrics::kFilterCheckboxSize, UiMetrics::kFilterCheckboxSize });
    lotteryLayout->AddSpacer(1.0f);
    lotteryLayout->AddChild(lotteryCloseRow, { UiMetrics::kPanelButtonRowWidth, UiMetrics::kPanelButtonRowHeight });

    // ============================================================
    // 5. READING GOAL PANEL
    // ============================================================
    m_ReadingGoalPanel = std::make_shared<Panel>(Anchor::Center, UiMetrics::kGoalPanelOffset, UiMetrics::kGoalPanelSize, "Reading Goal");
    m_ReadingGoalPanel->isVisible = false;

    auto goalLayout = m_ReadingGoalPanel->CreateContentLayout(
        FlexLayout::Direction::Vertical,
        UiMetrics::kPanelPaddingDefault,
        UiMetrics::kPanelGap,
        FlexLayout::CrossAlign::Start
    );

    m_GoalSummaryLabel = std::make_shared<Label>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ 340.0f, 60.0f }, "", 20, NookCol::UI_TEXT, true);
    m_GoalProgressLabel = std::make_shared<Label>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ 340.0f, 60.0f }, "", 20, NookCol::UI_TEXT_MUTED, true);

    auto adjustRow = makeButtonRow({ 340.0f, UiMetrics::kPanelButtonRowHeight });
    auto btnMinusFive = std::make_shared<Button>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kGoalButtonWidth, UiMetrics::kPanelButtonRowHeight }, "-5",
        [this]() { if (m_OnAdjustGoalTarget) m_OnAdjustGoalTarget(-5); }
    );
    auto btnMinusOne = std::make_shared<Button>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kGoalButtonWidth, UiMetrics::kPanelButtonRowHeight }, "-1",
        [this]() { if (m_OnAdjustGoalTarget) m_OnAdjustGoalTarget(-1); }
    );
    auto btnPlusOne = std::make_shared<Button>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kGoalButtonWidth, UiMetrics::kPanelButtonRowHeight }, "+1",
        [this]() { if (m_OnAdjustGoalTarget) m_OnAdjustGoalTarget(1); }
    );
    auto btnPlusFive = std::make_shared<Button>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kGoalButtonWidth, UiMetrics::kPanelButtonRowHeight }, "+5",
        [this]() { if (m_OnAdjustGoalTarget) m_OnAdjustGoalTarget(5); }
    );

    adjustRow->AddChild(btnMinusFive, { 0.0f, UiMetrics::kPanelButtonRowHeight }, 1.0f);
    adjustRow->AddChild(btnMinusOne, { 0.0f, UiMetrics::kPanelButtonRowHeight }, 1.0f);
    adjustRow->AddChild(btnPlusOne, { 0.0f, UiMetrics::kPanelButtonRowHeight }, 1.0f);
    adjustRow->AddChild(btnPlusFive, { 0.0f, UiMetrics::kPanelButtonRowHeight }, 1.0f);

    auto actionRow = makeButtonRow({ 340.0f, UiMetrics::kPanelButtonRowHeight });
    auto btnResetProgress = std::make_shared<Button>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ 160.0f, UiMetrics::kPanelButtonRowHeight }, "Reset Progress",
        [this]() { if (m_OnResetGoalProgress) m_OnResetGoalProgress(); }
    );
    auto btnCloseGoal = std::make_shared<Button>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ 120.0f, UiMetrics::kPanelButtonRowHeight }, "Close",
        [this]() { if (m_ReadingGoalPanel) m_ReadingGoalPanel->isVisible = false; }
    );

    actionRow->AddChild(btnResetProgress, { 0.0f, UiMetrics::kPanelButtonRowHeight }, 1.0f);
    actionRow->AddChild(btnCloseGoal, { 0.0f, UiMetrics::kPanelButtonRowHeight }, 1.0f);

    goalLayout->AddChild(m_GoalSummaryLabel, { 340.0f, 60.0f });
    goalLayout->AddChild(m_GoalProgressLabel, { 340.0f, 60.0f });
    goalLayout->AddSpacer(1.0f);
    goalLayout->AddChild(adjustRow, { 340.0f, UiMetrics::kPanelButtonRowHeight });
    goalLayout->AddChild(actionRow, { 340.0f, UiMetrics::kPanelButtonRowHeight });

    UpdateGoalPanelTexts();

    m_Widgets.push_back(m_AddPanel);
    m_Widgets.push_back(m_EditPanel);
    m_Widgets.push_back(m_LotteryPanel);
    m_Widgets.push_back(m_ReadingGoalPanel);

    m_CalendarWidget = std::make_shared<CalendarWidget>(
        Anchor::TopLeft,
        Vector2{ 220.0f, 220.0f },
        Vector2{ 320.0f, 300.0f }
    );
    m_CalendarWidget->isVisible = false;
    m_Widgets.push_back(m_CalendarWidget);
}

void UIManager::UpdateGoalPanelTexts()
{
    if (!m_GoalSummaryLabel || !m_GoalProgressLabel) return;
    if (!m_GetReadCount || !m_GetGoalTarget || !m_GetGoalProgress) return;

    const int readCount = std::max(0, m_GetReadCount());
    const int target = std::max(1, m_GetGoalTarget());
    const int progress = std::max(0, m_GetGoalProgress());
    const int remaining = std::max(0, target - progress);
    const float percent = std::clamp((float)progress / (float)target, 0.0f, 1.0f) * 100.0f;

    m_GoalSummaryLabel->SetText(std::format("Goal: {} books | Progress: {}/{} ({:.1f}%)", target, progress, target, percent));
    m_GoalProgressLabel->SetText(std::format("Read total: {} | Remaining: {}", readCount, remaining));
}