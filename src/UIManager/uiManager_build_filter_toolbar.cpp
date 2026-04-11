
// Responsible for building and managing filter panel and toolbar UI components.
// Contains functions for constructing the search/filter panel and main toolbar.


#include "uiManager.h"

#include "UI/button.h"
#include "UI/checkbox.h"
#include "UI/flexLayout.h"
#include "UI/label.h"
#include "UI/panel.h"
#include "UI/slider.h"
#include "UI/textBox.h"
#include "UI/textInput.h"
#include "colors.h"
#include "uiManager_internal.h"

#include <algorithm>
#include <format>

// Builds the filter panel with search, status filters, rating slider, and genre filter
void UIManager::BuildFilterPanel(const std::function<void(Status)>& onToggleStatus, const std::function<void(int)>& onSortBooks)
{
    m_SearchFilterPanel = std::make_shared<Panel>(Anchor::CenterRight, UiMetrics::kFilterPanelOffset, UiMetrics::kFilterPanelSize, "Search Filter");
    m_SearchFilterPanel->SetVisible(false);

    auto filterLayout = m_SearchFilterPanel->CreateContentLayout(
        FlexLayout::Direction::Vertical,
        UiMetrics::kPanelPaddingCompact,
        UiMetrics::kPanelGap,
        FlexLayout::CrossAlign::Start
    );

    m_CheckToRead = std::make_shared<Checkbox>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kFilterCheckboxSize, UiMetrics::kFilterCheckboxSize }, "To Read", true,
        [onToggleStatus](bool) { onToggleStatus(Status::ToRead); }
    );

    m_CheckReading = std::make_shared<Checkbox>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kFilterCheckboxSize, UiMetrics::kFilterCheckboxSize }, "Reading", true,
        [onToggleStatus](bool) { onToggleStatus(Status::Reading); }
    );

    m_CheckRead = std::make_shared<Checkbox>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kFilterCheckboxSize, UiMetrics::kFilterCheckboxSize }, "Read", true,
        [onToggleStatus](bool) { onToggleStatus(Status::Read); }
    );

    m_FilterRatingLabel = std::make_shared<Label>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kFilterControlWidth, 30.0f }, "Min. Rating: 0.0");

    m_FilterRatingSlider = std::make_shared<Slider>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kFilterControlWidth, 30.0f }, 0.0f, 5.0f, 0.0f,
        [this](float val) {
            m_FilterRatingLabel->SetText(std::format("Min. Rating: {:.1f}", val));
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

    auto composeFilterQuery = [this]() {
        auto trim = [](std::string value) {
            const size_t first = value.find_first_not_of(" \t\n\r");
            if (first == std::string::npos) return std::string{};
            const size_t last = value.find_last_not_of(" \t\n\r");
            return value.substr(first, last - first + 1);
        };

        std::string query;

        const float rating = m_FilterRatingSlider->value;
        if (rating > 0.0f) {
            query += "r>" + std::format("{:.1f}", rating);
        }

        const std::string genre = trim(m_FilterGenreInput->text);
        if (!genre.empty()) {
            if (!query.empty()) query += " | ";
            query += "g:" + genre;
        }

        if (m_FilterFinishedRangeState == 1) {
            if (!query.empty()) query += " | ";
            query += "fr:month";
        }
        else if (m_FilterFinishedRangeState == 2) {
            if (!query.empty()) query += " | ";
            query += "fr:year";
        }

        return query;
    };

    auto applyPreset = [this](const std::string& query, float rating, const std::string& genre, int finishedState) {
        m_FilterRatingSlider->value = std::clamp(rating, 0.0f, 5.0f);
        m_FilterRatingLabel->SetText(std::format("Min. Rating: {:.1f}", m_FilterRatingSlider->value));
        m_FilterGenreInput->text = genre;

        m_FilterFinishedRangeState = finishedState;
        if (m_FilterFinishedRangeState == 0) m_FilterFinishedRangeBtn->SetText("Finished: All");
        else if (m_FilterFinishedRangeState == 1) m_FilterFinishedRangeBtn->SetText("Finished: This Month");
        else m_FilterFinishedRangeBtn->SetText("Finished: This Year");

        m_ActiveFilterQuery = query;
    };

    auto helpLabel = std::make_shared<Label>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ UiMetrics::kFilterControlWidth, 80.0f },
        "Query help: duna | r>4 | g:fantasy | g:none | s:reading | fr:month",
        18,
        NookCol::UI_TEXT_MUTED,
        true
    );

    auto quickRow1 = CreateButtonRow({ UiMetrics::kFilterControlWidth, UiMetrics::kPanelButtonRowHeight }, 8.0f);
    auto quickHighRatingBtn = std::make_shared<Button>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ 0.0f, UiMetrics::kPanelButtonRowHeight },
        "High Rating",
        [applyPreset]() { applyPreset("r>4.0", 4.0f, "", 0); }
    );
    auto quickThisMonthBtn = std::make_shared<Button>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ 0.0f, UiMetrics::kPanelButtonRowHeight },
        "This Month",
        [applyPreset]() { applyPreset("fr:month", 0.0f, "", 1); }
    );
    quickRow1->AddChild(quickHighRatingBtn, { 0.0f, UiMetrics::kPanelButtonRowHeight }, 1.0f);
    quickRow1->AddChild(quickThisMonthBtn, { 0.0f, UiMetrics::kPanelButtonRowHeight }, 1.0f);

    auto quickRow2 = CreateButtonRow({ UiMetrics::kFilterControlWidth, UiMetrics::kPanelButtonRowHeight }, 8.0f);
    auto quickReadingBtn = std::make_shared<Button>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ 0.0f, UiMetrics::kPanelButtonRowHeight },
        "Reading",
        [applyPreset]() { applyPreset("s:reading", 0.0f, "", 0); }
    );
    auto quickMissingGenresBtn = std::make_shared<Button>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ 0.0f, UiMetrics::kPanelButtonRowHeight },
        "Missing Genres",
        [applyPreset]() { applyPreset("g:none", 0.0f, "", 0); }
    );
    quickRow2->AddChild(quickReadingBtn, { 0.0f, UiMetrics::kPanelButtonRowHeight }, 1.0f);
    quickRow2->AddChild(quickMissingGenresBtn, { 0.0f, UiMetrics::kPanelButtonRowHeight }, 1.0f);

    m_ApplyFiltersBtn = std::make_shared<Button>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ UiMetrics::kFilterControlWidth, UiMetrics::kPanelButtonRowHeight }, "Apply Filters",
        [this, composeFilterQuery]() {
            m_ActiveFilterQuery = composeFilterQuery();
        }
    );

    auto clearFiltersBtn = std::make_shared<Button>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ UiMetrics::kFilterControlWidth, UiMetrics::kPanelButtonRowHeight },
        "Clear Filters",
        [this]() {
            m_FilterRatingSlider->value = 0.0f;
            m_FilterRatingLabel->SetText("Min. Rating: 0.0");
            m_FilterGenreInput->Clear();
            m_FilterFinishedRangeState = 0;
            m_FilterFinishedRangeBtn->SetText("Finished: All");
            m_ActiveFilterQuery.clear();
        }
    );

    filterLayout->AddChild(m_CheckToRead, { UiMetrics::kFilterCheckboxSize, UiMetrics::kFilterCheckboxSize });
    filterLayout->AddChild(m_CheckReading, { UiMetrics::kFilterCheckboxSize, UiMetrics::kFilterCheckboxSize });
    filterLayout->AddChild(m_CheckRead, { UiMetrics::kFilterCheckboxSize, UiMetrics::kFilterCheckboxSize });
    filterLayout->AddChild(helpLabel, { UiMetrics::kFilterControlWidth, 80.0f });
    filterLayout->AddChild(quickRow1, { UiMetrics::kFilterControlWidth, UiMetrics::kPanelButtonRowHeight });
    filterLayout->AddChild(quickRow2, { UiMetrics::kFilterControlWidth, UiMetrics::kPanelButtonRowHeight });
    filterLayout->AddChild(m_FilterRatingLabel, { UiMetrics::kFilterControlWidth, 30.0f });
    filterLayout->AddChild(m_FilterRatingSlider, { UiMetrics::kFilterControlWidth, 30.0f });
    filterLayout->AddChild(m_FilterGenreLabel, { UiMetrics::kFilterControlWidth, 30.0f });
    filterLayout->AddChild(m_FilterGenreInput, { UiMetrics::kFilterControlWidth, 30.0f });
    filterLayout->AddChild(m_FilterFinishedRangeBtn, { UiMetrics::kFilterControlWidth, UiMetrics::kPanelButtonRowHeight });
    filterLayout->AddChild(m_FilterSortBtn, { UiMetrics::kFilterControlWidth, UiMetrics::kPanelButtonRowHeight });
    filterLayout->AddSpacer(1.0f);
    filterLayout->AddChild(clearFiltersBtn, { UiMetrics::kFilterControlWidth, UiMetrics::kPanelButtonRowHeight });
    filterLayout->AddChild(m_ApplyFiltersBtn, { UiMetrics::kFilterControlWidth, UiMetrics::kPanelButtonRowHeight });

    m_Widgets.push_back(m_SearchFilterPanel);
}

// Builds the main toolbar with action buttons for file operations, search, and panel toggles
void UIManager::BuildToolbar(
    const std::function<void()>& onSave,
    const std::function<void()>& onSaveAs,
    const std::function<void()>& onLoad,
    const std::function<void()>& onBackToMenu,
    const std::function<void()>& onUndo,
    const std::function<void()>& onRedo,
    const std::function<void()>& onToggleLayout)
{
    const float toolbarRowTop = UiMetrics::kToolbarRowTop;
    const float toolbarRowHeight = UiMetrics::kToolbarRowHeight;
    const float toolbarGap = UiMetrics::kToolbarGap;

    auto makeToolbarButton = [toolbarRowHeight](const std::string& label, std::function<void()> callback, float width) {
        return std::make_shared<Button>(Anchor::TopLeft, Vector2{ 0.0f, 0.0f }, Vector2{ width, toolbarRowHeight }, label, std::move(callback));
    };

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

    m_ToggleFiltersBtn = makeToolbarButton(NookConst::Text::kFilterToolbarGlyph,
        [this]() {
            static double lastClickTime = 0;
            if (GetTime() - lastClickTime > 0.2) {
                m_SearchFilterPanel->SetVisible(!m_SearchFilterPanel->IsVisible());
                lastClickTime = GetTime();
            }
        },
        UiMetrics::kToolbarFiltersWidth
    );
    m_ToolbarLayout->AddChild(m_ToggleFiltersBtn, { UiMetrics::kToolbarFiltersWidth, toolbarRowHeight });

    m_ToolbarLayout->AddChild(makeToolbarButton("Goals",
        [this]() {
            if (m_ReadingGoalPanel) {
                m_ReadingGoalPanel->SetVisible(!m_ReadingGoalPanel->IsVisible());
            }
        },
        UiMetrics::kToolbarGoalsWidth
    ), { UiMetrics::kToolbarGoalsWidth, toolbarRowHeight });

    m_OpenStatsBtn = makeToolbarButton("Stats",
        [this]() {
            if (m_AnalyticsPanel) {
                m_AnalyticsPanel->SetVisible(!m_AnalyticsPanel->IsVisible());
            }
        },
        UiMetrics::kToolbarStatsWidth
    );
    m_ToolbarLayout->AddChild(m_OpenStatsBtn, { UiMetrics::kToolbarStatsWidth, toolbarRowHeight });

    m_ToolbarLayout->AddChild(makeToolbarButton("Undo", onUndo, UiMetrics::kToolbarUndoWidth), { UiMetrics::kToolbarUndoWidth, toolbarRowHeight });
    m_ToolbarLayout->AddChild(makeToolbarButton("Redo", onRedo, UiMetrics::kToolbarRedoWidth), { UiMetrics::kToolbarRedoWidth, toolbarRowHeight });
    m_ToolbarLayout->AddChild(makeToolbarButton("Add Book", [this]() { m_AddPanel->SetVisible(true); }, UiMetrics::kToolbarAddBookWidth), { UiMetrics::kToolbarAddBookWidth, toolbarRowHeight });
    m_ToolbarLayout->AddChild(makeToolbarButton("Next Read",
        [this]() {
            m_LotteryPanel->SetVisible(true);
            m_IsLotteryRolling = true;
            m_LotteryTimer = 2.0f;
            m_LotterySpeedTimer = 0.0f;
            m_LotteryCloseBtn->SetVisible(false);
            m_LotteryText->SetText("Spinning...");
        },
        UiMetrics::kToolbarNextReadWidth
    ), { UiMetrics::kToolbarNextReadWidth, toolbarRowHeight });
    m_ToolbarLayout->AddChild(makeToolbarButton("Toggle Layout", onToggleLayout, UiMetrics::kToolbarToggleLayoutWidth), { UiMetrics::kToolbarToggleLayoutWidth, toolbarRowHeight });

    m_ToolbarLayout->AddSpacer(1.0f);

    m_OpenSettingsBtn = makeToolbarButton(
        NookConst::Text::kSettingsToolbarGlyph,
        [this]() {
            if (m_SettingsPanel) {
                m_SettingsPanel->SetVisible(!m_SettingsPanel->IsVisible());
            }
        },
        UiMetrics::kToolbarSettingsWidth
    );
    m_ToolbarLayout->AddChild(m_OpenSettingsBtn, { UiMetrics::kToolbarSettingsWidth, toolbarRowHeight });

    m_Widgets.push_back(m_ToolbarLayout);
}
