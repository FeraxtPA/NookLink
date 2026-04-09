
// Responsible for building all modal panels: book details, add/edit, goals, lottery.
// Each function constructs a complete panel with layout, controls, and callbacks.


#include "uiManager.h"

#include "colors.h"
#include "logging.h"
#include "uiManager_build_internal.h"
#include "uiManager_internal.h"

#include <cctype>
#include <format>
#include <unordered_set>

namespace {
std::string ToLowerAsciiCopy(std::string value)
{
    for (char& ch : value) {
        ch = (char)std::tolower((unsigned char)ch);
    }
    return value;
}

void AppendGenreToInput(const std::shared_ptr<TextInput>& input, const std::string& genre)
{
    if (!input) return;

    const std::string cleanGenre = UiBuildUtils::TrimCopy(genre);
    if (cleanGenre.empty()) return;

    std::vector<std::string> parsedGenres;
    std::unordered_set<std::string> seen;

    std::string raw = input->GetText();
    size_t start = 0;
    while (start <= raw.size()) {
        const size_t commaPos = raw.find(',', start);
        const size_t end = (commaPos == std::string::npos) ? raw.size() : commaPos;
        const std::string token = UiBuildUtils::TrimCopy(raw.substr(start, end - start));
        if (!token.empty()) {
            const std::string key = ToLowerAsciiCopy(token);
            if (seen.insert(key).second) {
                parsedGenres.push_back(token);
            }
        }

        if (commaPos == std::string::npos) break;
        start = commaPos + 1;
    }

    const std::string cleanKey = ToLowerAsciiCopy(cleanGenre);
    if (seen.insert(cleanKey).second) {
        parsedGenres.push_back(cleanGenre);
    }

    std::string joined;
    for (size_t i = 0; i < parsedGenres.size(); ++i) {
        if (i > 0) joined += ", ";
        joined += parsedGenres[i];
    }
    input->text = joined;
}
}

// Builds the book details panel displaying information about a selected book
void UIManager::BuildBookDetailsPanel()
{
    m_BookDetailsPanel = std::make_shared<Panel>(Anchor::CenterLeft, UiMetrics::kDetailsPanelOffset, UiMetrics::kDetailsPanelSize, "Book Info");
    m_BookDetailsPanel->isVisible = false;

    auto detailsLayout = m_BookDetailsPanel->CreateContentLayout(
        FlexLayout::Direction::Vertical,
        UiMetrics::kPanelPaddingDetails,
        UiMetrics::kPanelGap,
        FlexLayout::CrossAlign::Start
    );

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

    auto detailsButtonRow = CreateButtonRow({ UiMetrics::kDetailsButtonRowWidth, UiMetrics::kPanelButtonRowHeight });
    detailsButtonRow->AddChild(m_DetailsEditBtn, { 0.0f, UiMetrics::kPanelButtonRowHeight }, 1.0f);
    detailsButtonRow->AddChild(m_DetailsCloseBtn, { 0.0f, UiMetrics::kPanelButtonRowHeight }, 1.0f);

    detailsLayout->AddChild(m_DetailsText, { UiMetrics::kDetailsTextWidth, UiMetrics::kDetailsTextHeight });
    detailsLayout->AddSpacer(1.0f);
    detailsLayout->AddChild(detailsButtonRow, { UiMetrics::kDetailsButtonRowWidth, UiMetrics::kPanelButtonRowHeight });

    m_Widgets.push_back(m_BookDetailsPanel);
}

// Builds the add book panel with input fields for book metadata and date selectors
void UIManager::BuildAddPanel(const AddBookCallback& onAddBook)
{
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
    m_AddGenreDropdown = std::make_shared<Dropdown>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight },
        std::vector<std::string>{ "Select existing genre..." },
        0,
        [this](int selectedIndex, const std::string& selectedText) {
            if (selectedIndex <= 0) return;
            AppendGenreToInput(m_AddGenres, selectedText);
            if (m_AddGenreDropdown) {
                m_AddGenreDropdown->SetSelectedIndex(0, false);
            }
        }
    );
    m_AddRating = std::make_shared<TextInput>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight }, "Rating (0.0 - 5.0)");
    m_AddStartedDate = std::make_shared<TextInput>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight }, "Started Reading (DD.MM.YYYY)");
    m_AddFinishedDate = std::make_shared<TextInput>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight }, "Finished Reading (DD.MM.YYYY)");
    auto addStartedRow = CreateDateInputRow(m_AddStartedDate);
    auto addFinishedRow = CreateDateInputRow(m_AddFinishedDate);
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
            UiBuildUtils::AutoFillDatesForStatus(selected, m_AddStartedDate, m_AddFinishedDate);
        }
    });

    auto btnCreate = std::make_shared<Button>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ 100.0f, UiMetrics::kPanelButtonRowHeight }, "Create",
        [this, onAddBook]() {
            // Validate required fields first to avoid partial/invalid entities.
            const std::string t = UiBuildUtils::TrimCopy(m_AddTitle->GetText());
            const std::string a = UiBuildUtils::TrimCopy(m_AddAuthor->GetText());
            if (t.empty() || a.empty()) {
                ShowNotification("Title and Author are required.");
                Log::Warn("Add book blocked: missing Title or Author");
                return;
            }

            float r = 0.0f;
            if (!UiBuildUtils::TryParseRating(m_AddRating->GetText(), r)) {
                ShowNotification("Rating must be a valid number.");
                Log::Warn("Add book blocked: invalid rating format");
                return;
            }

            if (r < 0.0f || r > 5.0f) {
                ShowNotification("Rating must be between 0.0 and 5.0.");
                Log::Warn("Add book blocked: rating out of range");
                return;
            }

            const std::string started = UiBuildUtils::TrimCopy(m_AddStartedDate->GetText());
            const std::string finished = UiBuildUtils::TrimCopy(m_AddFinishedDate->GetText());
            if (!UiBuildUtils::IsValidDateDDMMYYYY(started) || !UiBuildUtils::IsValidDateDDMMYYYY(finished)) {
                ShowNotification("Dates must be in DD.MM.YYYY format.");
                Log::Warn("Add book blocked: invalid date format");
                return;
            }

            Status s = Status::ToRead;
            if (*m_AddStatusState == 1) s = Status::Reading;
            if (*m_AddStatusState == 2) s = Status::Read;

            UiBuildUtils::AutoFillDatesForStatus(s, m_AddStartedDate, m_AddFinishedDate);

            // Re-read normalized values after autofill so downstream callback receives final state.
            const std::string normalizedStarted = UiBuildUtils::TrimCopy(m_AddStartedDate->GetText());
            const std::string normalizedFinished = UiBuildUtils::TrimCopy(m_AddFinishedDate->GetText());
            if (!normalizedStarted.empty() && !normalizedFinished.empty() && UiBuildUtils::CompareDateDDMMYYYY(normalizedStarted, normalizedFinished) > 0) {
                ShowNotification("Finished date cannot be earlier than started date.");
                Log::Warn("Add book blocked: finished date earlier than started date");
                return;
            }

            onAddBook(t, a, m_AddGenres->GetText(), r, s, m_AddNotes->GetText(), normalizedStarted, normalizedFinished);
            m_AddTitle->Clear();
            m_AddAuthor->Clear();
            m_AddGenres->Clear();
            if (m_AddGenreDropdown) m_AddGenreDropdown->SetSelectedIndex(0, false);
            m_AddRating->Clear();
            m_AddStartedDate->Clear();
            m_AddFinishedDate->Clear();
            m_AddNotes->Clear();
            if (m_CalendarWidget) m_CalendarWidget->Close();
            m_ActiveDateInput.reset();
            m_AddPanel->isVisible = false;
        }
    );

    auto btnCancelAdd = std::make_shared<Button>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ 100.0f, UiMetrics::kPanelButtonRowHeight }, "Cancel",
        [this]() {
            m_AddTitle->Clear();
            m_AddAuthor->Clear();
            m_AddGenres->Clear();
            if (m_AddGenreDropdown) m_AddGenreDropdown->SetSelectedIndex(0, false);
            m_AddRating->Clear();
            m_AddStartedDate->Clear();
            m_AddFinishedDate->Clear();
            m_AddNotes->Clear();
            if (m_CalendarWidget) m_CalendarWidget->Close();
            m_ActiveDateInput.reset();
            m_AddPanel->isVisible = false;
        }
    );

    auto addButtonRow = CreateButtonRow({ UiMetrics::kPanelButtonRowWidth, UiMetrics::kPanelButtonRowHeight });
    addButtonRow->AddChild(btnCreate, { 0.0f, UiMetrics::kPanelButtonRowHeight }, 1.0f);
    addButtonRow->AddChild(btnCancelAdd, { 0.0f, UiMetrics::kPanelButtonRowHeight }, 1.0f);

    addLayout->AddChild(m_AddTitle, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    addLayout->AddChild(m_AddAuthor, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    addLayout->AddChild(m_AddGenres, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    addLayout->AddChild(m_AddGenreDropdown, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    addLayout->AddChild(m_AddRating, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    addLayout->AddChild(addStartedRow, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    addLayout->AddChild(addFinishedRow, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    addLayout->AddChild(m_AddNotes, { UiMetrics::kPanelWideFieldWidth, 100.0f });
    addLayout->AddChild(m_AddStatusBtn, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelButtonRowHeight });
    addLayout->AddSpacer(1.0f);
    addLayout->AddChild(addButtonRow, { UiMetrics::kPanelButtonRowWidth, UiMetrics::kPanelButtonRowHeight });

    m_Widgets.push_back(m_AddPanel);
}

// Builds the edit book panel with input fields for updating existing book data
void UIManager::BuildEditPanel(const EditBookCallback& onEditBook)
{
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
    m_EditGenreDropdown = std::make_shared<Dropdown>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight },
        std::vector<std::string>{ "Select existing genre..." },
        0,
        [this](int selectedIndex, const std::string& selectedText) {
            if (selectedIndex <= 0) return;
            AppendGenreToInput(m_EditGenres, selectedText);
            if (m_EditGenreDropdown) {
                m_EditGenreDropdown->SetSelectedIndex(0, false);
            }
        }
    );
    m_EditRating = std::make_shared<TextInput>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight }, "Rating");
    m_EditStartedDate = std::make_shared<TextInput>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight }, "Started Reading (DD.MM.YYYY)");
    m_EditFinishedDate = std::make_shared<TextInput>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight }, "Finished Reading (DD.MM.YYYY)");
    auto editStartedRow = CreateDateInputRow(m_EditStartedDate);
    auto editFinishedRow = CreateDateInputRow(m_EditFinishedDate);
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
            UiBuildUtils::AutoFillDatesForStatus(selected, m_EditStartedDate, m_EditFinishedDate);
        }
    });

    auto btnUpdate = std::make_shared<Button>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ 100.0f, UiMetrics::kPanelButtonRowHeight }, "Update",
        [this, onEditBook, editState]() {
            // Same validation pipeline as create flow, but preserving current book identity.
            const std::string title = UiBuildUtils::TrimCopy(m_EditTitle->GetText());
            const std::string author = UiBuildUtils::TrimCopy(m_EditAuthor->GetText());
            if (title.empty() || author.empty()) {
                ShowNotification("Title and Author are required.");
                Log::Warn("Edit book blocked: missing Title or Author");
                return;
            }

            float r = 0.0f;
            if (!UiBuildUtils::TryParseRating(m_EditRating->GetText(), r)) {
                ShowNotification("Rating must be a valid number.");
                Log::Warn("Edit book blocked: invalid rating format");
                return;
            }

            if (r < 0.0f || r > 5.0f) {
                ShowNotification("Rating must be between 0.0 and 5.0.");
                Log::Warn("Edit book blocked: rating out of range");
                return;
            }

            const std::string started = UiBuildUtils::TrimCopy(m_EditStartedDate->GetText());
            const std::string finished = UiBuildUtils::TrimCopy(m_EditFinishedDate->GetText());
            if (!UiBuildUtils::IsValidDateDDMMYYYY(started) || !UiBuildUtils::IsValidDateDDMMYYYY(finished)) {
                ShowNotification("Dates must be in DD.MM.YYYY format.");
                Log::Warn("Edit book blocked: invalid date format");
                return;
            }

            Status s = Status::ToRead;
            if (*editState == 1) s = Status::Reading;
            if (*editState == 2) s = Status::Read;

            UiBuildUtils::AutoFillDatesForStatus(s, m_EditStartedDate, m_EditFinishedDate);

            const std::string normalizedStarted = UiBuildUtils::TrimCopy(m_EditStartedDate->GetText());
            const std::string normalizedFinished = UiBuildUtils::TrimCopy(m_EditFinishedDate->GetText());
            if (!normalizedStarted.empty() && !normalizedFinished.empty() && UiBuildUtils::CompareDateDDMMYYYY(normalizedStarted, normalizedFinished) > 0) {
                ShowNotification("Finished date cannot be earlier than started date.");
                Log::Warn("Edit book blocked: finished date earlier than started date");
                return;
            }

            onEditBook(m_EditingBookId, title, author, m_EditGenres->GetText(), r, s, m_EditNotes->GetText(), normalizedStarted, normalizedFinished);
            if (m_CalendarWidget) m_CalendarWidget->Close();
            m_ActiveDateInput.reset();
            if (m_EditGenreDropdown) m_EditGenreDropdown->SetSelectedIndex(0, false);
            m_EditPanel->isVisible = false;
        }
    );

    auto btnCancelEdit = std::make_shared<Button>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ 100.0f, UiMetrics::kPanelButtonRowHeight }, "Cancel",
        [this]() {
            if (m_CalendarWidget) m_CalendarWidget->Close();
            m_ActiveDateInput.reset();
            if (m_EditGenreDropdown) m_EditGenreDropdown->SetSelectedIndex(0, false);
            m_EditPanel->isVisible = false;
        }
    );

    auto editButtonRow = CreateButtonRow({ UiMetrics::kPanelButtonRowWidth, UiMetrics::kPanelButtonRowHeight });
    editButtonRow->AddChild(btnUpdate, { 0.0f, UiMetrics::kPanelButtonRowHeight }, 1.0f);
    editButtonRow->AddChild(btnCancelEdit, { 0.0f, UiMetrics::kPanelButtonRowHeight }, 1.0f);

    editLayout->AddChild(m_EditTitle, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    editLayout->AddChild(m_EditAuthor, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    editLayout->AddChild(m_EditGenres, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    editLayout->AddChild(m_EditGenreDropdown, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    editLayout->AddChild(m_EditRating, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    editLayout->AddChild(editStartedRow, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    editLayout->AddChild(editFinishedRow, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    editLayout->AddChild(m_EditNotes, { UiMetrics::kPanelWideFieldWidth, 100.0f });
    editLayout->AddChild(m_EditStatusBtn, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelButtonRowHeight });
    editLayout->AddSpacer(1.0f);
    editLayout->AddChild(editButtonRow, { UiMetrics::kPanelButtonRowWidth, UiMetrics::kPanelButtonRowHeight });

    m_Widgets.push_back(m_EditPanel);
}

// Builds the lottery panel for random book selection feature
void UIManager::BuildLotteryPanel()
{
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

    auto lotteryCloseRow = CreateButtonRow({ UiMetrics::kPanelButtonRowWidth, UiMetrics::kPanelButtonRowHeight });
    lotteryCloseRow->AddSpacer(1.0f);
    lotteryCloseRow->AddChild(m_LotteryCloseBtn, { 100.0f, UiMetrics::kPanelButtonRowHeight });
    lotteryCloseRow->AddSpacer(1.0f);

    lotteryLayout->AddChild(m_LotteryText, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kLotteryTextHeight });
    lotteryLayout->AddChild(m_LotteryAutoRead, { UiMetrics::kFilterCheckboxSize, UiMetrics::kFilterCheckboxSize });
    lotteryLayout->AddSpacer(1.0f);
    lotteryLayout->AddChild(lotteryCloseRow, { UiMetrics::kPanelButtonRowWidth, UiMetrics::kPanelButtonRowHeight });

    m_Widgets.push_back(m_LotteryPanel);
}

// Builds the reading goal panel for tracking annual reading targets and progress
void UIManager::BuildReadingGoalPanel()
{
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

    auto adjustRow = CreateButtonRow({ 340.0f, UiMetrics::kPanelButtonRowHeight });
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

    auto actionRow = CreateButtonRow({ 340.0f, UiMetrics::kPanelButtonRowHeight });
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

    m_Widgets.push_back(m_ReadingGoalPanel);
}

void UIManager::BuildAnalyticsPanel()
{
    m_AnalyticsPanel = std::make_shared<Panel>(Anchor::Center, UiMetrics::kAnalyticsPanelOffset, UiMetrics::kAnalyticsPanelSize, "Statistics");
    m_AnalyticsPanel->isVisible = false;

    auto analyticsLayout = m_AnalyticsPanel->CreateContentLayout(
        FlexLayout::Direction::Vertical,
        UiMetrics::kPanelPaddingDefault,
        8.0f,
        FlexLayout::CrossAlign::Start
    );

    m_AnalyticsSummaryLabel = std::make_shared<Label>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ UiMetrics::kAnalyticsLabelWidth, 70.0f },
        "Total books: 0",
        22,
        NookCol::UI_TEXT,
        true
    );

    m_AnalyticsRatingLabel = std::make_shared<Label>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ UiMetrics::kAnalyticsLabelWidth, 60.0f },
        "Average rating: -",
        20,
        NookCol::UI_TEXT_MUTED,
        true
    );

    m_AnalyticsStatusLabel = std::make_shared<Label>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ UiMetrics::kAnalyticsLabelWidth, 90.0f },
        "Status split",
        20,
        NookCol::UI_TEXT,
        true
    );

    m_AnalyticsTimeLabel = std::make_shared<Label>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ UiMetrics::kAnalyticsLabelWidth, 70.0f },
        "Finished this month/year",
        20,
        NookCol::UI_TEXT,
        true
    );

    m_AnalyticsGenreLabel = std::make_shared<Label>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ UiMetrics::kAnalyticsLabelWidth, 130.0f },
        "Top genres",
        20,
        NookCol::UI_TEXT,
        true
    );

    auto closeBtn = std::make_shared<Button>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ 220.0f, UiMetrics::kPanelButtonRowHeight },
        "Close Statistics",
        [this]() {
            if (m_AnalyticsPanel) {
                m_AnalyticsPanel->isVisible = false;
            }
        }
    );

    auto closeRow = CreateButtonRow({ UiMetrics::kAnalyticsLabelWidth, UiMetrics::kPanelButtonRowHeight });
    closeRow->AddSpacer(1.0f);
    closeRow->AddChild(closeBtn, { 220.0f, UiMetrics::kPanelButtonRowHeight });
    closeRow->AddSpacer(1.0f);

    analyticsLayout->AddChild(m_AnalyticsSummaryLabel, { UiMetrics::kAnalyticsLabelWidth, 70.0f });
    analyticsLayout->AddChild(m_AnalyticsRatingLabel, { UiMetrics::kAnalyticsLabelWidth, 60.0f });
    analyticsLayout->AddChild(m_AnalyticsStatusLabel, { UiMetrics::kAnalyticsLabelWidth, 90.0f });
    analyticsLayout->AddChild(m_AnalyticsTimeLabel, { UiMetrics::kAnalyticsLabelWidth, 70.0f });
    analyticsLayout->AddChild(m_AnalyticsGenreLabel, { UiMetrics::kAnalyticsLabelWidth, 130.0f });
    analyticsLayout->AddChild(closeRow, { UiMetrics::kAnalyticsLabelWidth, UiMetrics::kPanelButtonRowHeight });

    m_Widgets.push_back(m_AnalyticsPanel);
}

void UIManager::BuildSettingsPanel(
    const std::function<std::string(int)>& onSelectTheme,
    const std::function<int()>& getCurrentThemeIndex,
    const std::function<int()>& getThemeCount,
    const std::function<std::string(int)>& getThemeNameByIndex,
    const std::function<void(float)>& onSetLayoutDensity,
    const std::function<float()>& getLayoutDensity)
{
    m_SettingsPanel = std::make_shared<Panel>(Anchor::Center, UiMetrics::kSettingsPanelOffset, UiMetrics::kSettingsPanelSize, "Settings");
    m_SettingsPanel->isVisible = false;

    auto settingsLayout = m_SettingsPanel->CreateContentLayout(
        FlexLayout::Direction::Vertical,
        UiMetrics::kPanelPaddingCompact,
        UiMetrics::kPanelGap,
        FlexLayout::CrossAlign::Start
    );

    auto themeHelpLabel = std::make_shared<Label>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ UiMetrics::kSettingsControlWidth, 86.0f },
        "Theme preset\nUse dropdown to choose a preset instantly.",
        20,
        NookCol::UI_TEXT_MUTED,
        true
    );

    std::vector<std::string> themeNames;
    const int themeCount = getThemeCount ? std::max(0, getThemeCount()) : 0;
    themeNames.reserve((size_t)themeCount);
    for (int i = 0; i < themeCount; ++i) {
        if (getThemeNameByIndex) {
            themeNames.push_back(getThemeNameByIndex(i));
        }
        else {
            themeNames.push_back(std::format("Theme {}", i + 1));
        }
    }

    const int currentThemeIndex = getCurrentThemeIndex ? getCurrentThemeIndex() : 0;

    m_ThemeDropdown = std::make_shared<Dropdown>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ UiMetrics::kSettingsControlWidth, UiMetrics::kPanelButtonRowHeight },
        themeNames,
        currentThemeIndex,
        [this, onSelectTheme](int selectedIndex, const std::string&) {
            if (!onSelectTheme) return;
            const std::string appliedThemeName = onSelectTheme(selectedIndex);
            if (!appliedThemeName.empty()) {
                ShowNotification("Theme switched to " + appliedThemeName);
            }
        }
    );

    const float initialDensity = getLayoutDensity ? std::clamp(getLayoutDensity(), 0.3f, 1.6f) : 1.0f;

    auto densityHelpLabel = std::make_shared<Label>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ UiMetrics::kSettingsControlWidth, 64.0f },
        "Graph layout density\nLower = compact, higher = more spread clusters.",
        20,
        NookCol::UI_TEXT_MUTED,
        true
    );

    m_LayoutDensityValueLabel = std::make_shared<Label>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ UiMetrics::kSettingsControlWidth, 28.0f },
        std::format("Density: {:.1f}x", initialDensity),
        20,
        NookCol::UI_TEXT,
        false
    );

    m_LayoutDensitySlider = std::make_shared<Slider>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ UiMetrics::kSettingsControlWidth, UiMetrics::kPanelButtonRowHeight },
        0.3f,
        1.6f,
        initialDensity,
        [this, onSetLayoutDensity](float value) {
            if (onSetLayoutDensity) {
                onSetLayoutDensity(value);
            }
            if (m_LayoutDensityValueLabel) {
                m_LayoutDensityValueLabel->SetText(std::format("Density: {:.1f}x", value));
            }
        }
    );

    auto closeBtn = std::make_shared<Button>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ 240.0f, UiMetrics::kPanelButtonRowHeight },
        "Close Settings",
        [this]() {
            if (m_SettingsPanel) {
                m_SettingsPanel->isVisible = false;
            }
        }
    );

    auto closeRow = CreateButtonRow({ UiMetrics::kSettingsControlWidth, UiMetrics::kPanelButtonRowHeight });
    closeRow->AddSpacer(1.0f);
    closeRow->AddChild(closeBtn, { 240.0f, UiMetrics::kPanelButtonRowHeight });
    closeRow->AddSpacer(1.0f);

    settingsLayout->AddChild(themeHelpLabel, { UiMetrics::kSettingsControlWidth, 86.0f });
    settingsLayout->AddChild(m_ThemeDropdown, { UiMetrics::kSettingsControlWidth, UiMetrics::kPanelButtonRowHeight });
    settingsLayout->AddChild(densityHelpLabel, { UiMetrics::kSettingsControlWidth, 64.0f });
    settingsLayout->AddChild(m_LayoutDensityValueLabel, { UiMetrics::kSettingsControlWidth, 28.0f });
    settingsLayout->AddChild(m_LayoutDensitySlider, { UiMetrics::kSettingsControlWidth, UiMetrics::kPanelButtonRowHeight });
    settingsLayout->AddSpacer(1.0f);
    settingsLayout->AddChild(closeRow, { UiMetrics::kSettingsControlWidth, UiMetrics::kPanelButtonRowHeight });

    m_Widgets.push_back(m_SettingsPanel);
}
