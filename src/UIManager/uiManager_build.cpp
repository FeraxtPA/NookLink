
// Main UI interface initialization and update functions.
// Coordinates the building of all UI panels and components.


#include "uiManager.h"

#include "UI/button.h"
#include "UI/calendarWidget.h"
#include "UI/flexLayout.h"
#include "UI/label.h"
#include "UI/textInput.h"
#include "uiManager_internal.h"

#include <algorithm>
#include <format>

// Builds the complete UI interface by initializing all panels and widgets
void UIManager::BuildInterface(
    std::function<void()> onSave,
    std::function<void()> onSaveAs,
    std::function<void()> onLoad,
    std::function<void()> onImportCsv,
    std::function<void()> onExportCsv,
    std::function<void()> onBackToMenu,
    std::function<void()> onUndo,
    std::function<void()> onRedo,
    std::function<std::string(int)> onSelectTheme,
    std::function<int()> getCurrentThemeIndex,
    std::function<int()> getThemeCount,
    std::function<std::string(int)> getThemeNameByIndex,
    std::function<void(float)> onSetLayoutDensity,
    std::function<float()> getLayoutDensity,
    AddBookCallback onAddBook,
    EditBookCallback onEditBook,
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

    BuildFilterPanel(onToggleStatus, onSortBooks);
    BuildReadingGoalPanel();
    BuildAnalyticsPanel();
    BuildLotteryPanel();
    BuildBulkGenreAssignPanel();
    BuildAddPanel(onAddBook);
    BuildEditPanel(onEditBook);
    BuildBookDetailsPanel();
    BuildToolbar(onSave, onSaveAs, onLoad, onBackToMenu, onUndo, onRedo, onToggleLayout);
    // Build settings after toolbar so it renders above the toolbar shell.
    BuildSettingsPanel(onSelectTheme, getCurrentThemeIndex, getThemeCount, getThemeNameByIndex, onSetLayoutDensity, getLayoutDensity, onImportCsv, onExportCsv);

    UpdateGoalPanelTexts();

    m_CalendarWidget = std::make_shared<CalendarWidget>(
        Anchor::TopLeft,
        Vector2{ 220.0f, 220.0f },
        Vector2{ 320.0f, 300.0f }
    );
    m_CalendarWidget->SetVisible(false);
    m_Widgets.push_back(m_CalendarWidget);
}

// Creates a horizontal button row layout with specified size and gap
std::shared_ptr<FlexLayout> UIManager::CreateButtonRow(Vector2 size, float gap)
{
    return std::make_shared<FlexLayout>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        size,
        FlexLayout::Direction::Horizontal,
        Vector2{ 0.0f, 0.0f },
        gap,
        FlexLayout::CrossAlign::Stretch
    );
}

// Creates a date input row with a text input field and date picker button
std::shared_ptr<FlexLayout> UIManager::CreateDateInputRow(const std::shared_ptr<TextInput>& input)
{
    constexpr float kDateButtonWidth = 76.0f;
    constexpr float kDateButtonGap = 14.0f;

    auto row = CreateButtonRow({ UiMetrics::kPanelWideFieldWidth, UiMetrics::kPanelInputHeight }, kDateButtonGap);

    auto pickButton = std::make_shared<Button>(
        Anchor::TopLeft,
        Vector2{ 0.0f, 0.0f },
        Vector2{ kDateButtonWidth, UiMetrics::kPanelInputHeight },
        "Pick",
        [this, input]() {
            OpenCalendarFor(input);
        }
    );

    row->AddChild(input, { UiMetrics::kPanelWideFieldWidth - kDateButtonWidth - kDateButtonGap, UiMetrics::kPanelInputHeight }, 1.0f);
    row->AddChild(pickButton, { kDateButtonWidth, UiMetrics::kPanelInputHeight });
    return row;
}

// Updates goal panel labels with current reading goal progress and statistics
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
