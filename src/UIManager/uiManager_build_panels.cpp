
// Responsible for building all modal panels: book details, add/edit, goals, lottery.
// Each function constructs a complete panel with layout, controls, and callbacks.


#include "uiManager.h"

#include "UI/button.h"
#include "UI/checkbox.h"
#include "UI/dropdown.h"
#include "UI/flexLayout.h"
#include "UI/label.h"
#include "UI/panel.h"
#include "UI/slider.h"
#include "UI/calendarWidget.h"
#include "UI/textBox.h"
#include "UI/textInput.h"
#include "colors.h"
#include "date_utils.h"
#include "googleBooksClient.h"
#include "logging.h"
#include "uiManager_internal.h"
#include "validation.h"

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

    const std::string cleanGenre = Validation::TrimCopy(genre);
    if (cleanGenre.empty()) return;

    std::vector<std::string> parsedGenres;
    std::unordered_set<std::string> seen;

    std::string raw = input->GetText();
    size_t start = 0;
    while (start <= raw.size()) {
        const size_t commaPos = raw.find(',', start);
        const size_t end = (commaPos == std::string::npos) ? raw.size() : commaPos;
        const std::string token = Validation::TrimCopy(raw.substr(start, end - start));
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

void AutoFillDatesForStatus(Status status, const std::shared_ptr<TextInput>& startedInput, const std::shared_ptr<TextInput>& finishedInput)
{
    if (!startedInput || !finishedInput) {
        return;
    }

    const std::string today = DateUtils::GetTodayDateDDMMYYYY();

    if ((status == Status::Reading || status == Status::Read) && startedInput->text.empty()) {
        startedInput->text = today;
    }

    if (status == Status::Read && finishedInput->text.empty()) {
        finishedInput->text = today;
    }
}
}

// Builds the book details panel displaying information about a selected book
void UIManager::BuildBookDetailsPanel()
{
    m_BookDetailsPanel = std::make_shared<Panel>(Anchor::CenterLeft, UiMetrics::kDetailsPanelOffset, UiMetrics::kDetailsPanelSize, "Book Info");
    m_BookDetailsPanel->SetVisible(false);

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
                m_BookDetailsPanel->SetVisible(false);
            }
        }
    );

    m_DetailsCloseBtn = std::make_shared<Button>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ 120.0f, UiMetrics::kPanelButtonRowHeight }, "Close",
        [this]() { m_BookDetailsPanel->SetVisible(false); }
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
    m_AddPanel = std::make_shared<Panel>(Anchor::Center, UiMetrics::kEditorPanelOffset, Vector2{ UiMetrics::kEditorPanelSize.x, 860.0f }, "Add New Book");
    m_AddPanel->SetVisible(false);

    auto addLayout = m_AddPanel->CreateContentLayout(
        FlexLayout::Direction::Vertical,
        UiMetrics::kPanelPaddingDefault,
        UiMetrics::kPanelGap,
        FlexLayout::CrossAlign::Center
    );

    m_AddTitle = std::make_shared<TextInput>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight }, "Title");
    m_AddIsbn = std::make_shared<TextInput>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight }, "ISBN (10/13)");
    m_AddIsbn->maxLength = 20;
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
    m_AddPages = std::make_shared<TextInput>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight }, "Pages (optional)");
    m_AddPages->maxLength = 6;
    m_AddPublishedDate = std::make_shared<TextInput>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight }, "Published (YYYY or YYYY-MM-DD)");
    m_AddRating = std::make_shared<TextInput>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight }, "Rating (0.0 - 5.0)");
    m_AddStartedDate = std::make_shared<TextInput>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight }, "Started (DD.MM.YYYY)");
    m_AddFinishedDate = std::make_shared<TextInput>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight }, "Finished (DD.MM.YYYY)");
    auto addStartedRow = CreateDateInputRow(m_AddStartedDate);
    auto addFinishedRow = CreateDateInputRow(m_AddFinishedDate);
    m_AddNotes = std::make_shared<TextBox>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, 100.0f }, "Notes");

    m_AddStatusState = std::make_shared<int>(0);
    m_AddStatusBtn = std::make_shared<Button>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelButtonRowHeight }, "Status: To Read", []() {});
    m_AddFetchIsbnBtn = std::make_shared<Button>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelButtonRowHeight },
        "Fetch by ISBN",
        [this]() {
            const std::string isbn = Validation::TrimCopy(m_AddIsbn ? m_AddIsbn->GetText() : "");
            if (isbn.empty()) {
                if (m_AddIsbn) m_AddIsbn->SetValidationError(true);
                ShowNotification("Please enter ISBN first.");
                return;
            }

            if (!Validation::IsValidIsbnFormat(isbn)) {
                if (m_AddIsbn) m_AddIsbn->SetValidationError(true);
                ShowNotification("Invalid ISBN format (expected 10 or 13 chars).");
                return;
            }

            if (m_AddIsbn) m_AddIsbn->SetValidationError(false);

            GoogleBooksClient client;
            GoogleBookData fetched;
            std::string fetchError;
            if (!client.FetchByIsbn(isbn, fetched, fetchError)) {
                ShowNotification("ISBN fetch failed: " + fetchError);
                Log::Warn("ISBN fetch failed: " + fetchError);
                return;
            }

            if (fetched.hasTitle) m_AddTitle->text = fetched.title;
            if (fetched.hasAuthor) m_AddAuthor->text = fetched.author;
            if (fetched.hasGenres) m_AddGenres->text = fetched.genres;
            if (fetched.hasPageCount) m_AddPages->text = std::to_string(fetched.pageCount);
            if (fetched.hasPublishedDate) m_AddPublishedDate->text = fetched.publishedDate;

            if (m_AddGenreDropdown) m_AddGenreDropdown->SetSelectedIndex(0, false);
            ShowNotification("Book info fetched from Google Books.");
        }
    );

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
            // Validate required fields first to avoid partial/invalid entities.
            const std::string t = Validation::TrimCopy(m_AddTitle->GetText());
            const std::string a = Validation::TrimCopy(m_AddAuthor->GetText());
            if (t.empty() || a.empty()) {
                ShowNotification("Title and Author are required.");
                Log::Warn("Add book blocked: missing Title or Author");
                return;
            }

            float r = 0.0f;
            if (!Validation::TryParseRating(m_AddRating->GetText(), r)) {
                ShowNotification("Rating must be a valid number.");
                Log::Warn("Add book blocked: invalid rating format");
                return;
            }

            int pageCount = 0;
            if (!Validation::TryParsePageCount(m_AddPages->GetText(), pageCount)) {
                ShowNotification("Pages must be a valid whole number.");
                Log::Warn("Add book blocked: invalid page count format");
                return;
            }

            const std::string published = Validation::TrimCopy(m_AddPublishedDate->GetText());
            if (!Validation::IsValidPublishedDate(published)) {
                ShowNotification("Published date must be YYYY, YYYY-MM or YYYY-MM-DD.");
                Log::Warn("Add book blocked: invalid published date format");
                return;
            }

            if (r < 0.0f || r > 5.0f) {
                ShowNotification("Rating must be between 0.0 and 5.0.");
                Log::Warn("Add book blocked: rating out of range");
                return;
            }

            const std::string started = Validation::TrimCopy(m_AddStartedDate->GetText());
            const std::string finished = Validation::TrimCopy(m_AddFinishedDate->GetText());
            if (!DateUtils::IsValidDateDDMMYYYY(started) || !DateUtils::IsValidDateDDMMYYYY(finished)) {
                ShowNotification("Dates must be in DD.MM.YYYY format.");
                Log::Warn("Add book blocked: invalid date format");
                return;
            }

            Status s = Status::ToRead;
            if (*m_AddStatusState == 1) s = Status::Reading;
            if (*m_AddStatusState == 2) s = Status::Read;

            AutoFillDatesForStatus(s, m_AddStartedDate, m_AddFinishedDate);

            // Re-read normalized values after autofill so downstream callback receives final state.
            const std::string normalizedStarted = Validation::TrimCopy(m_AddStartedDate->GetText());
            const std::string normalizedFinished = Validation::TrimCopy(m_AddFinishedDate->GetText());
            if (!normalizedStarted.empty() && !normalizedFinished.empty() && DateUtils::CompareDateDDMMYYYY(normalizedStarted, normalizedFinished) > 0) {
                ShowNotification("Finished date cannot be earlier than started date.");
                Log::Warn("Add book blocked: finished date earlier than started date");
                return;
            }

            onAddBook(t, a, m_AddGenres->GetText(), pageCount, published, r, s, m_AddNotes->GetText(), normalizedStarted, normalizedFinished);
            m_AddTitle->Clear();
            m_AddIsbn->Clear();
            m_AddIsbn->SetValidationError(false);
            m_AddAuthor->Clear();
            m_AddGenres->Clear();
            if (m_AddGenreDropdown) m_AddGenreDropdown->SetSelectedIndex(0, false);
            m_AddPages->Clear();
            m_AddPublishedDate->Clear();
            m_AddRating->Clear();
            m_AddStartedDate->Clear();
            m_AddFinishedDate->Clear();
            m_AddNotes->Clear();
            if (m_CalendarWidget) m_CalendarWidget->Close();
            m_ActiveDateInput.reset();
            m_AddPanel->SetVisible(false);
        }
    );

    auto btnCancelAdd = std::make_shared<Button>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ 100.0f, UiMetrics::kPanelButtonRowHeight }, "Cancel",
        [this]() {
            m_AddTitle->Clear();
            m_AddIsbn->Clear();
            m_AddIsbn->SetValidationError(false);
            m_AddAuthor->Clear();
            m_AddGenres->Clear();
            if (m_AddGenreDropdown) m_AddGenreDropdown->SetSelectedIndex(0, false);
            m_AddPages->Clear();
            m_AddPublishedDate->Clear();
            m_AddRating->Clear();
            m_AddStartedDate->Clear();
            m_AddFinishedDate->Clear();
            m_AddNotes->Clear();
            if (m_CalendarWidget) m_CalendarWidget->Close();
            m_ActiveDateInput.reset();
            m_AddPanel->SetVisible(false);
        }
    );

    auto addButtonRow = CreateButtonRow({ UiMetrics::kPanelButtonRowWidth, UiMetrics::kPanelButtonRowHeight });
    addButtonRow->AddChild(btnCreate, { 0.0f, UiMetrics::kPanelButtonRowHeight }, 1.0f);
    addButtonRow->AddChild(btnCancelAdd, { 0.0f, UiMetrics::kPanelButtonRowHeight }, 1.0f);

    addLayout->AddChild(m_AddIsbn, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    addLayout->AddChild(m_AddFetchIsbnBtn, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelButtonRowHeight });
    addLayout->AddChild(m_AddTitle, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    addLayout->AddChild(m_AddAuthor, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    addLayout->AddChild(m_AddGenres, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    addLayout->AddChild(m_AddGenreDropdown, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    addLayout->AddChild(m_AddPages, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    addLayout->AddChild(m_AddPublishedDate, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
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
    m_EditPanel->SetVisible(false);

    auto editLayout = m_EditPanel->CreateContentLayout(
        FlexLayout::Direction::Vertical,
        UiMetrics::kPanelPaddingDefault,
        UiMetrics::kPanelGap,
        FlexLayout::CrossAlign::Center
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
    m_EditPages = std::make_shared<TextInput>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight }, "Pages");
    m_EditPages->maxLength = 6;
    m_EditPublishedDate = std::make_shared<TextInput>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight }, "Published (YYYY or YYYY-MM-DD)");
    m_EditRating = std::make_shared<TextInput>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight }, "Rating");
    m_EditStartedDate = std::make_shared<TextInput>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight }, "Started (DD.MM.YYYY)");
    m_EditFinishedDate = std::make_shared<TextInput>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight }, "Finished (DD.MM.YYYY)");
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
            AutoFillDatesForStatus(selected, m_EditStartedDate, m_EditFinishedDate);
        }
    });

    auto btnUpdate = std::make_shared<Button>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ 100.0f, UiMetrics::kPanelButtonRowHeight }, "Update",
        [this, onEditBook, editState]() {
            // Same validation pipeline as create flow, but preserving current book identity.
            const std::string title = Validation::TrimCopy(m_EditTitle->GetText());
            const std::string author = Validation::TrimCopy(m_EditAuthor->GetText());
            if (title.empty() || author.empty()) {
                ShowNotification("Title and Author are required.");
                Log::Warn("Edit book blocked: missing Title or Author");
                return;
            }

            float r = 0.0f;
            if (!Validation::TryParseRating(m_EditRating->GetText(), r)) {
                ShowNotification("Rating must be a valid number.");
                Log::Warn("Edit book blocked: invalid rating format");
                return;
            }

            int pageCount = 0;
            if (!Validation::TryParsePageCount(m_EditPages->GetText(), pageCount)) {
                ShowNotification("Pages must be a valid whole number.");
                Log::Warn("Edit book blocked: invalid page count format");
                return;
            }

            const std::string published = Validation::TrimCopy(m_EditPublishedDate->GetText());
            if (!Validation::IsValidPublishedDate(published)) {
                ShowNotification("Published date must be YYYY, YYYY-MM or YYYY-MM-DD.");
                Log::Warn("Edit book blocked: invalid published date format");
                return;
            }

            if (r < 0.0f || r > 5.0f) {
                ShowNotification("Rating must be between 0.0 and 5.0.");
                Log::Warn("Edit book blocked: rating out of range");
                return;
            }

            const std::string started = Validation::TrimCopy(m_EditStartedDate->GetText());
            const std::string finished = Validation::TrimCopy(m_EditFinishedDate->GetText());
            if (!DateUtils::IsValidDateDDMMYYYY(started) || !DateUtils::IsValidDateDDMMYYYY(finished)) {
                ShowNotification("Dates must be in DD.MM.YYYY format.");
                Log::Warn("Edit book blocked: invalid date format");
                return;
            }

            Status s = Status::ToRead;
            if (*editState == 1) s = Status::Reading;
            if (*editState == 2) s = Status::Read;

            AutoFillDatesForStatus(s, m_EditStartedDate, m_EditFinishedDate);

            const std::string normalizedStarted = Validation::TrimCopy(m_EditStartedDate->GetText());
            const std::string normalizedFinished = Validation::TrimCopy(m_EditFinishedDate->GetText());
            if (!normalizedStarted.empty() && !normalizedFinished.empty() && DateUtils::CompareDateDDMMYYYY(normalizedStarted, normalizedFinished) > 0) {
                ShowNotification("Finished date cannot be earlier than started date.");
                Log::Warn("Edit book blocked: finished date earlier than started date");
                return;
            }

            onEditBook(m_EditingBookId, title, author, m_EditGenres->GetText(), pageCount, published, r, s, m_EditNotes->GetText(), normalizedStarted, normalizedFinished);
            if (m_CalendarWidget) m_CalendarWidget->Close();
            m_ActiveDateInput.reset();
            if (m_EditGenreDropdown) m_EditGenreDropdown->SetSelectedIndex(0, false);
            m_EditPanel->SetVisible(false);
        }
    );

    auto btnCancelEdit = std::make_shared<Button>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ 100.0f, UiMetrics::kPanelButtonRowHeight }, "Cancel",
        [this]() {
            if (m_CalendarWidget) m_CalendarWidget->Close();
            m_ActiveDateInput.reset();
            if (m_EditGenreDropdown) m_EditGenreDropdown->SetSelectedIndex(0, false);
            m_EditPanel->SetVisible(false);
        }
    );

    auto editButtonRow = CreateButtonRow({ UiMetrics::kPanelButtonRowWidth, UiMetrics::kPanelButtonRowHeight });
    editButtonRow->AddChild(btnUpdate, { 0.0f, UiMetrics::kPanelButtonRowHeight }, 1.0f);
    editButtonRow->AddChild(btnCancelEdit, { 0.0f, UiMetrics::kPanelButtonRowHeight }, 1.0f);

    editLayout->AddChild(m_EditTitle, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    editLayout->AddChild(m_EditAuthor, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    editLayout->AddChild(m_EditGenres, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    editLayout->AddChild(m_EditGenreDropdown, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    editLayout->AddChild(m_EditPages, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    editLayout->AddChild(m_EditPublishedDate, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
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
    m_LotteryPanel->SetVisible(false);

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
        [this]() { m_LotteryPanel->SetVisible(false); }
    );
    m_LotteryCloseBtn->SetVisible(false);

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

void UIManager::BuildBulkGenreAssignPanel()
{
    m_BulkGenreAssignPanel = std::make_shared<Panel>(
        Anchor::Center,
        Vector2{ 0.0f, -140.0f },
        Vector2{ 500.0f, 420.0f },
        "Bulk Edit Selection"
    );
    m_BulkGenreAssignPanel->SetVisible(false);

    auto layout = m_BulkGenreAssignPanel->CreateContentLayout(
        FlexLayout::Direction::Vertical,
        UiMetrics::kPanelPaddingDefault,
        UiMetrics::kPanelGap,
        FlexLayout::CrossAlign::Center
    );

    m_BulkGenreAssignInfoLabel = std::make_shared<Label>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ UiMetrics::kPanelWideFieldWidth, 28.0f },
        "Selected books: 0",
        20,
        NookCol::UI_TEXT_MUTED,
        false
    );

    m_BulkGenreAssignInput = std::make_shared<TextInput>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight },
        "Genre (add/remove)"
    );

    m_BulkGenreAssignStatusState = std::make_shared<int>(0);
    m_BulkGenreAssignStatusBtn = std::make_shared<Button>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelButtonRowHeight },
        "Status: (unchanged)",
        [this]() {
            if (!m_BulkGenreAssignStatusState || !m_BulkGenreAssignStatusBtn) return;
            *m_BulkGenreAssignStatusState = (*m_BulkGenreAssignStatusState + 1) % 4;
            if (*m_BulkGenreAssignStatusState == 0) m_BulkGenreAssignStatusBtn->SetText("Status: (unchanged)");
            else if (*m_BulkGenreAssignStatusState == 1) m_BulkGenreAssignStatusBtn->SetText("Status: To Read");
            else if (*m_BulkGenreAssignStatusState == 2) m_BulkGenreAssignStatusBtn->SetText("Status: Reading");
            else m_BulkGenreAssignStatusBtn->SetText("Status: Read");
        }
    );

    m_BulkGenreAssignPublishedInput = std::make_shared<TextInput>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight },
        "Published (YYYY or YYYY-MM-DD)"
    );

    m_BulkGenreAssignPagesInput = std::make_shared<TextInput>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight },
        "Pages"
    );
    m_BulkGenreAssignPagesInput->maxLength = 6;

    m_BulkGenreAssignOnlyIfEmpty = std::make_shared<Checkbox>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ UiMetrics::kFilterCheckboxSize, UiMetrics::kFilterCheckboxSize },
        "Apply only if empty"
    );

    m_BulkGenreAssignApplyBtn = std::make_shared<Button>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ 120.0f, UiMetrics::kPanelButtonRowHeight },
        "Apply",
        [this]() {
            if (!m_BulkGenreAssignInput || !m_BulkGenreAssignStatusState || !m_BulkGenreAssignPublishedInput || !m_BulkGenreAssignPagesInput) return;

            BulkEditRequest request;
            request.removeGenre = false;
            request.genre = Validation::TrimCopy(m_BulkGenreAssignInput->GetText());
            request.applyOnlyIfEmpty = m_BulkGenreAssignOnlyIfEmpty && m_BulkGenreAssignOnlyIfEmpty->checked;

            if (*m_BulkGenreAssignStatusState > 0) {
                request.hasStatus = true;
                request.status = *m_BulkGenreAssignStatusState == 1 ? Status::ToRead : (*m_BulkGenreAssignStatusState == 2 ? Status::Reading : Status::Read);
            }

            request.published = Validation::TrimCopy(m_BulkGenreAssignPublishedInput->GetText());
            if (!request.published.empty()) {
                if (!Validation::IsValidPublishedDate(request.published)) {
                    ShowNotification("Published date must be YYYY, YYYY-MM or YYYY-MM-DD.");
                    return;
                }
                request.hasPublished = true;
            }

            const std::string pageText = Validation::TrimCopy(m_BulkGenreAssignPagesInput->GetText());
            if (!pageText.empty()) {
                int parsedPages = 0;
                if (!Validation::TryParsePageCount(pageText, parsedPages)) {
                    ShowNotification("Pages must be a valid whole number.");
                    return;
                }
                request.hasPages = true;
                request.pages = parsedPages;
            }

            if (request.genre.empty() && !request.hasStatus && !request.hasPublished && !request.hasPages) {
                ShowNotification("Fill at least one field to apply bulk edit.");
                return;
            }

            m_PendingBulkEditRequest = request;
            m_HasPendingBulkEditRequest = true;
            m_BulkGenreAssignInput->Clear();
            if (m_BulkGenreAssignPanel) m_BulkGenreAssignPanel->SetVisible(false);
        }
    );

    m_BulkGenreAssignRemoveBtn = std::make_shared<Button>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ 120.0f, UiMetrics::kPanelButtonRowHeight },
        "Remove",
        [this]() {
            if (!m_BulkGenreAssignInput) return;
            const std::string genre = Validation::TrimCopy(m_BulkGenreAssignInput->GetText());
            if (genre.empty()) {
                ShowNotification("Genre cannot be empty.");
                return;
            }

            BulkEditRequest request;
            request.removeGenre = true;
            request.genre = genre;
            m_PendingBulkEditRequest = request;
            m_HasPendingBulkEditRequest = true;
            m_BulkGenreAssignInput->Clear();
            if (m_BulkGenreAssignPanel) m_BulkGenreAssignPanel->SetVisible(false);
        }
    );

    m_BulkGenreAssignCancelBtn = std::make_shared<Button>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ 120.0f, UiMetrics::kPanelButtonRowHeight },
        "Cancel",
        [this]() {
            if (m_BulkGenreAssignInput) m_BulkGenreAssignInput->Clear();
            if (m_BulkGenreAssignPublishedInput) m_BulkGenreAssignPublishedInput->Clear();
            if (m_BulkGenreAssignPagesInput) m_BulkGenreAssignPagesInput->Clear();
            if (m_BulkGenreAssignOnlyIfEmpty) m_BulkGenreAssignOnlyIfEmpty->checked = false;
            if (m_BulkGenreAssignStatusState) *m_BulkGenreAssignStatusState = 0;
            if (m_BulkGenreAssignStatusBtn) m_BulkGenreAssignStatusBtn->SetText("Status: (unchanged)");
            if (m_BulkGenreAssignPanel) m_BulkGenreAssignPanel->SetVisible(false);
        }
    );

    auto buttonRow = CreateButtonRow({ UiMetrics::kPanelButtonRowWidth, UiMetrics::kPanelButtonRowHeight });
    buttonRow->AddChild(m_BulkGenreAssignApplyBtn, { 0.0f, UiMetrics::kPanelButtonRowHeight }, 1.0f);
    buttonRow->AddChild(m_BulkGenreAssignRemoveBtn, { 0.0f, UiMetrics::kPanelButtonRowHeight }, 1.0f);
    buttonRow->AddChild(m_BulkGenreAssignCancelBtn, { 0.0f, UiMetrics::kPanelButtonRowHeight }, 1.0f);

    layout->AddChild(m_BulkGenreAssignInfoLabel, { UiMetrics::kPanelWideFieldWidth, 28.0f });
    layout->AddChild(m_BulkGenreAssignInput, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    layout->AddChild(m_BulkGenreAssignStatusBtn, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelButtonRowHeight });
    layout->AddChild(m_BulkGenreAssignPublishedInput, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    layout->AddChild(m_BulkGenreAssignPagesInput, { UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight });
    layout->AddChild(m_BulkGenreAssignOnlyIfEmpty, { UiMetrics::kFilterCheckboxSize, UiMetrics::kFilterCheckboxSize });
    layout->AddSpacer(1.0f);
    layout->AddChild(buttonRow, { UiMetrics::kPanelButtonRowWidth, UiMetrics::kPanelButtonRowHeight });

    m_Widgets.push_back(m_BulkGenreAssignPanel);
}

// Builds the reading goal panel for tracking annual reading targets and progress
void UIManager::BuildReadingGoalPanel()
{
    m_ReadingGoalPanel = std::make_shared<Panel>(Anchor::Center, UiMetrics::kGoalPanelOffset, UiMetrics::kGoalPanelSize, "Reading Goal");
    m_ReadingGoalPanel->SetVisible(false);

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
        [this]() { if (m_ReadingGoalPanel) m_ReadingGoalPanel->SetVisible(false); }
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
    const float panelWidth = std::clamp((float)m_ScreenWidth * 0.82f, 900.0f, 1700.0f);
    const float panelHeight = std::clamp((float)m_ScreenHeight * 0.78f, 700.0f, 980.0f);
    const float analyticsLabelWidth = panelWidth - 80.0f;

    m_AnalyticsPanel = std::make_shared<Panel>(Anchor::Center, UiMetrics::kAnalyticsPanelOffset, Vector2{ panelWidth, panelHeight }, "Statistics");
    m_AnalyticsPanel->SetVisible(false);

    auto analyticsLayout = m_AnalyticsPanel->CreateContentLayout(
        FlexLayout::Direction::Vertical,
        UiMetrics::kPanelPaddingDefault,
        8.0f,
        FlexLayout::CrossAlign::Start
    );

    m_AnalyticsOverviewLabel = std::make_shared<Label>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ 240.0f, 96.0f },
        "Overview",
        20,
        NookCol::UI_TEXT,
        true
    );
    m_AnalyticsTopGenresListLabel = std::make_shared<Label>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ 240.0f, 120.0f },
        "Top genres",
        18,
        NookCol::UI_TEXT_MUTED,
        true
    );
    m_AnalyticsTopAuthorsListLabel = std::make_shared<Label>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ 240.0f, 120.0f },
        "Top authors",
        18,
        NookCol::UI_TEXT_MUTED,
        true
    );
    m_AnalyticsTopRatedListLabel = std::make_shared<Label>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ 240.0f, 120.0f },
        "Top rated books",
        18,
        NookCol::UI_TEXT_MUTED,
        true
    );
    m_AnalyticsPagesLabel = std::make_shared<Label>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ 240.0f, 96.0f },
        "Pages",
        18,
        NookCol::UI_TEXT_MUTED,
        true
    );
    m_AnalyticsPublicationLabel = std::make_shared<Label>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ 240.0f, 96.0f },
        "Publication",
        18,
        NookCol::UI_TEXT_MUTED,
        true
    );

    m_AnalyticsPanel->AddChild(m_AnalyticsOverviewLabel);
    m_AnalyticsPanel->AddChild(m_AnalyticsTopGenresListLabel);
    m_AnalyticsPanel->AddChild(m_AnalyticsTopAuthorsListLabel);
    m_AnalyticsPanel->AddChild(m_AnalyticsTopRatedListLabel);
    m_AnalyticsPanel->AddChild(m_AnalyticsPagesLabel);
    m_AnalyticsPanel->AddChild(m_AnalyticsPublicationLabel);

    m_AnalyticsRightPanelMode = AnalyticsRightPanelMode::StatusPie;
    auto goNextAnalyticsMode = [this]() {
        if (m_AnalyticsRightPanelMode == AnalyticsRightPanelMode::StatusPie) m_AnalyticsRightPanelMode = AnalyticsRightPanelMode::RatingProgress;
        else if (m_AnalyticsRightPanelMode == AnalyticsRightPanelMode::RatingProgress) m_AnalyticsRightPanelMode = AnalyticsRightPanelMode::RatingHistogram;
        else if (m_AnalyticsRightPanelMode == AnalyticsRightPanelMode::RatingHistogram) m_AnalyticsRightPanelMode = AnalyticsRightPanelMode::ReadingMomentum;
        else if (m_AnalyticsRightPanelMode == AnalyticsRightPanelMode::ReadingMomentum) m_AnalyticsRightPanelMode = AnalyticsRightPanelMode::AuthorDistribution;
        else if (m_AnalyticsRightPanelMode == AnalyticsRightPanelMode::AuthorDistribution) m_AnalyticsRightPanelMode = AnalyticsRightPanelMode::GenreTreemap;
        else if (m_AnalyticsRightPanelMode == AnalyticsRightPanelMode::GenreTreemap) m_AnalyticsRightPanelMode = AnalyticsRightPanelMode::PageDistribution;
        else if (m_AnalyticsRightPanelMode == AnalyticsRightPanelMode::PageDistribution) m_AnalyticsRightPanelMode = AnalyticsRightPanelMode::PublicationTimeline;
        else m_AnalyticsRightPanelMode = AnalyticsRightPanelMode::StatusPie;
    };

    auto goPrevAnalyticsMode = [this]() {
        if (m_AnalyticsRightPanelMode == AnalyticsRightPanelMode::StatusPie) m_AnalyticsRightPanelMode = AnalyticsRightPanelMode::PublicationTimeline;
        else if (m_AnalyticsRightPanelMode == AnalyticsRightPanelMode::RatingProgress) m_AnalyticsRightPanelMode = AnalyticsRightPanelMode::StatusPie;
        else if (m_AnalyticsRightPanelMode == AnalyticsRightPanelMode::RatingHistogram) m_AnalyticsRightPanelMode = AnalyticsRightPanelMode::RatingProgress;
        else if (m_AnalyticsRightPanelMode == AnalyticsRightPanelMode::ReadingMomentum) m_AnalyticsRightPanelMode = AnalyticsRightPanelMode::RatingHistogram;
        else if (m_AnalyticsRightPanelMode == AnalyticsRightPanelMode::AuthorDistribution) m_AnalyticsRightPanelMode = AnalyticsRightPanelMode::ReadingMomentum;
        else if (m_AnalyticsRightPanelMode == AnalyticsRightPanelMode::GenreTreemap) m_AnalyticsRightPanelMode = AnalyticsRightPanelMode::AuthorDistribution;
        else if (m_AnalyticsRightPanelMode == AnalyticsRightPanelMode::PageDistribution) m_AnalyticsRightPanelMode = AnalyticsRightPanelMode::GenreTreemap;
        else m_AnalyticsRightPanelMode = AnalyticsRightPanelMode::PageDistribution;
    };

    m_AnalyticsChartModePrevBtn = std::make_shared<Button>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ 120.0f, UiMetrics::kPanelButtonRowHeight },
        "<< Prev",
        [goPrevAnalyticsMode]() {
            goPrevAnalyticsMode();
        }
    );

    m_AnalyticsChartModeBtn = std::make_shared<Button>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ 120.0f, UiMetrics::kPanelButtonRowHeight },
        "Next >>",
        [goNextAnalyticsMode]() {
            goNextAnalyticsMode();
        }
    );

    m_AnalyticsPageBinsBtn = std::make_shared<Button>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ 140.0f, UiMetrics::kPanelButtonRowHeight },
        std::format("Page bins: {}", m_AnalyticsPageBinCount),
        [this]() {
            m_AnalyticsPageBinCount += 1;
            if (m_AnalyticsPageBinCount > 10) m_AnalyticsPageBinCount = 4;
            if (m_AnalyticsPageBinsBtn) {
                m_AnalyticsPageBinsBtn->SetText(std::format("Page bins: {}", m_AnalyticsPageBinCount));
            }
            MarkAnalyticsDirty();
        }
    );
    m_AnalyticsPageBinsBtn->SetVisible(false);

    auto closeBtn = std::make_shared<Button>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ 220.0f, UiMetrics::kPanelButtonRowHeight },
        "Close Statistics",
        [this]() {
            if (m_AnalyticsPanel) {
                m_AnalyticsPanel->SetVisible(false);
            }
        }
    );

    auto closeRow = CreateButtonRow({ analyticsLabelWidth, UiMetrics::kPanelButtonRowHeight });
    closeRow->AddSpacer(1.0f);
    closeRow->AddChild(closeBtn, { 220.0f, UiMetrics::kPanelButtonRowHeight });
    closeRow->AddSpacer(0.2f);
    closeRow->AddChild(m_AnalyticsChartModePrevBtn, { 120.0f, UiMetrics::kPanelButtonRowHeight });
    closeRow->AddChild(m_AnalyticsChartModeBtn, { 120.0f, UiMetrics::kPanelButtonRowHeight });
    closeRow->AddChild(m_AnalyticsPageBinsBtn, { 140.0f, UiMetrics::kPanelButtonRowHeight });
    closeRow->AddSpacer(1.0f);

    analyticsLayout->AddSpacer(1.0f);
    analyticsLayout->AddChild(closeRow, { analyticsLabelWidth, UiMetrics::kPanelButtonRowHeight });

    m_Widgets.push_back(m_AnalyticsPanel);
}

void UIManager::BuildSettingsPanel(
    const std::function<std::string(int)>& onSelectTheme,
    const std::function<int()>& getCurrentThemeIndex,
    const std::function<int()>& getThemeCount,
    const std::function<std::string(int)>& getThemeNameByIndex,
    const std::function<void(float)>& onSetLayoutDensity,
    const std::function<float()>& getLayoutDensity,
    const std::function<void()>& onImportCsv,
    const std::function<void()>& onExportCsv)
{
    m_SettingsPanel = std::make_shared<Panel>(Anchor::Center, UiMetrics::kSettingsPanelOffset, UiMetrics::kSettingsPanelSize, "Settings");
    m_SettingsPanel->SetVisible(false);

    auto settingsLayout = m_SettingsPanel->CreateContentLayout(
        FlexLayout::Direction::Vertical,
        UiMetrics::kPanelPaddingCompact,
        UiMetrics::kPanelGap,
        FlexLayout::CrossAlign::Start
    );

    auto dataHelpLabel = std::make_shared<Label>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ UiMetrics::kSettingsControlWidth, 64.0f },
        "Data tools\nImport from Goodreads CSV or export current library as CSV.",
        20,
        NookCol::UI_TEXT_MUTED,
        true
    );

    auto importCsvBtn = std::make_shared<Button>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ UiMetrics::kSettingsControlWidth, UiMetrics::kPanelButtonRowHeight },
        "Import Goodreads CSV",
        [onImportCsv]() {
            if (onImportCsv) onImportCsv();
        }
    );

    auto exportCsvBtn = std::make_shared<Button>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ UiMetrics::kSettingsControlWidth, UiMetrics::kPanelButtonRowHeight },
        "Export Library CSV",
        [onExportCsv]() {
            if (onExportCsv) onExportCsv();
        }
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
    m_ThemeDropdown->SetMaxVisibleOptions(6);

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

    m_SettingsHelpExpanded = false;
    m_SettingsHelpToggleBtn = std::make_shared<Button>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ UiMetrics::kSettingsControlWidth, UiMetrics::kPanelButtonRowHeight },
        "? Controls Help",
        [this]() {
            m_SettingsHelpExpanded = !m_SettingsHelpExpanded;
            if (m_SettingsHelpToggleBtn) {
                m_SettingsHelpToggleBtn->SetText(m_SettingsHelpExpanded ? "? Hide Controls Help" : "? Controls Help");
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
                m_SettingsPanel->SetVisible(false);
            }
            m_SettingsHelpExpanded = false;
            if (m_SettingsHelpToggleBtn) {
                m_SettingsHelpToggleBtn->SetText("? Controls Help");
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
    settingsLayout->AddChild(dataHelpLabel, { UiMetrics::kSettingsControlWidth, 64.0f });
    settingsLayout->AddChild(importCsvBtn, { UiMetrics::kSettingsControlWidth, UiMetrics::kPanelButtonRowHeight });
    settingsLayout->AddChild(exportCsvBtn, { UiMetrics::kSettingsControlWidth, UiMetrics::kPanelButtonRowHeight });
    settingsLayout->AddChild(m_SettingsHelpToggleBtn, { UiMetrics::kSettingsControlWidth, UiMetrics::kPanelButtonRowHeight });
    settingsLayout->AddSpacer(1.0f);
    settingsLayout->AddChild(closeRow, { UiMetrics::kSettingsControlWidth, UiMetrics::kPanelButtonRowHeight });

    m_Widgets.push_back(m_SettingsPanel);
}

