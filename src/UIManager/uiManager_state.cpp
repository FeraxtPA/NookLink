#include "uiManager.h"

#include "UI/button.h"
#include "UI/calendarWidget.h"
#include "UI/panel.h"
#include "UI/textInput.h"

#include <algorithm>
#include <functional>

namespace {
class UiCloseStack {
public:
    template <typename T, size_t N>
    static void TrackItem(
        T item,
        bool isOpen,
        std::array<bool, N>& openStates,
        std::vector<T>& history)
    {
        const size_t index = (size_t)item;
        const bool wasOpen = openStates[index];
        if (isOpen == wasOpen) {
            return;
        }

        openStates[index] = isOpen;
        if (isOpen) {
            history.push_back(item);
        }
        else {
            history.erase(
                std::remove(history.begin(), history.end(), item),
                history.end());
        }
    }

    template <typename T, typename IsOpenFn>
    static bool PopNextOpen(
        std::vector<T>& history,
        IsOpenFn&& isOpen,
        T& outItem)
    {
        while (!history.empty()) {
            const T item = history.back();
            history.pop_back();

            if (!isOpen(item)) {
                continue;
            }

            outItem = item;
            return true;
        }

        return false;
    }
};
}

void UIManager::TrackClosableItem(ClosableUiItem item, bool isOpen)
{
    UiCloseStack::TrackItem(item, isOpen, m_LastClosableOpenStates, m_ClosableHistory);
}

bool UIManager::IsClosableItemOpen(ClosableUiItem item) const
{
    switch (item) {
    case ClosableUiItem::NodeContextMenu: return m_NodeContextMenuVisible;
    case ClosableUiItem::SettingsHelp: return m_SettingsHelpExpanded && m_SettingsPanel && m_SettingsPanel->IsVisible();
    case ClosableUiItem::Calendar: return m_CalendarWidget && m_CalendarWidget->IsOpen();
    case ClosableUiItem::BookDetails: return m_BookDetailsPanel && m_BookDetailsPanel->IsVisible();
    case ClosableUiItem::EditPanel: return m_EditPanel && m_EditPanel->IsVisible();
    case ClosableUiItem::AddPanel: return m_AddPanel && m_AddPanel->IsVisible();
    case ClosableUiItem::SearchFilter: return m_SearchFilterPanel && m_SearchFilterPanel->IsVisible();
    case ClosableUiItem::Settings: return m_SettingsPanel && m_SettingsPanel->IsVisible();
    case ClosableUiItem::Analytics: return m_AnalyticsPanel && m_AnalyticsPanel->IsVisible();
    case ClosableUiItem::ReadingGoal: return m_ReadingGoalPanel && m_ReadingGoalPanel->IsVisible();
    case ClosableUiItem::Lottery: return m_LotteryPanel && m_LotteryPanel->IsVisible();
    case ClosableUiItem::BulkGenreAssign: return m_BulkGenreAssignPanel && m_BulkGenreAssignPanel->IsVisible();
    default: return false;
    }
}

void UIManager::CloseClosableItem(ClosableUiItem item)
{
    switch (item) {
    case ClosableUiItem::NodeContextMenu:
        CloseNodeContextMenu();
        break;
    case ClosableUiItem::SettingsHelp:
        m_SettingsHelpExpanded = false;
        if (m_SettingsHelpToggleBtn) {
            m_SettingsHelpToggleBtn->SetText("? Controls Help");
        }
        break;
    case ClosableUiItem::Calendar:
        if (m_CalendarWidget) m_CalendarWidget->Close();
        m_ActiveDateInput.reset();
        break;
    case ClosableUiItem::BookDetails:
        if (m_BookDetailsPanel) m_BookDetailsPanel->SetVisible(false);
        break;
    case ClosableUiItem::EditPanel:
        if (m_EditPanel) m_EditPanel->SetVisible(false);
        break;
    case ClosableUiItem::AddPanel:
        if (m_AddPanel) m_AddPanel->SetVisible(false);
        break;
    case ClosableUiItem::SearchFilter:
        if (m_SearchFilterPanel) m_SearchFilterPanel->SetVisible(false);
        break;
    case ClosableUiItem::Settings:
        if (m_SettingsPanel) m_SettingsPanel->SetVisible(false);
        m_SettingsHelpExpanded = false;
        if (m_SettingsHelpToggleBtn) {
            m_SettingsHelpToggleBtn->SetText("? Controls Help");
        }
        break;
    case ClosableUiItem::Analytics:
        if (m_AnalyticsPanel) m_AnalyticsPanel->SetVisible(false);
        break;
    case ClosableUiItem::ReadingGoal:
        if (m_ReadingGoalPanel) m_ReadingGoalPanel->SetVisible(false);
        break;
    case ClosableUiItem::Lottery:
        if (m_LotteryPanel) m_LotteryPanel->SetVisible(false);
        break;
    case ClosableUiItem::BulkGenreAssign:
        if (m_BulkGenreAssignPanel) m_BulkGenreAssignPanel->SetVisible(false);
        if (m_BulkGenreAssignInput) m_BulkGenreAssignInput->Clear();
        if (m_BulkGenreAssignPublishedInput) m_BulkGenreAssignPublishedInput->Clear();
        if (m_BulkGenreAssignPagesInput) m_BulkGenreAssignPagesInput->Clear();
        break;
    default:
        break;
    }
}

void UIManager::SyncClosableHistory()
{
    TrackClosableItem(ClosableUiItem::NodeContextMenu, m_NodeContextMenuVisible);
    TrackClosableItem(ClosableUiItem::SettingsHelp, m_SettingsHelpExpanded && m_SettingsPanel && m_SettingsPanel->IsVisible());
    TrackClosableItem(ClosableUiItem::Calendar, m_CalendarWidget && m_CalendarWidget->IsOpen());
    TrackClosableItem(ClosableUiItem::BookDetails, m_BookDetailsPanel && m_BookDetailsPanel->IsVisible());
    TrackClosableItem(ClosableUiItem::EditPanel, m_EditPanel && m_EditPanel->IsVisible());
    TrackClosableItem(ClosableUiItem::AddPanel, m_AddPanel && m_AddPanel->IsVisible());
    TrackClosableItem(ClosableUiItem::SearchFilter, m_SearchFilterPanel && m_SearchFilterPanel->IsVisible());
    TrackClosableItem(ClosableUiItem::Settings, m_SettingsPanel && m_SettingsPanel->IsVisible());
    TrackClosableItem(ClosableUiItem::Analytics, m_AnalyticsPanel && m_AnalyticsPanel->IsVisible());
    TrackClosableItem(ClosableUiItem::ReadingGoal, m_ReadingGoalPanel && m_ReadingGoalPanel->IsVisible());
    TrackClosableItem(ClosableUiItem::Lottery, m_LotteryPanel && m_LotteryPanel->IsVisible());
    TrackClosableItem(ClosableUiItem::BulkGenreAssign, m_BulkGenreAssignPanel && m_BulkGenreAssignPanel->IsVisible());
}

bool UIManager::HandleEscapeCloseRequest()
{
    SyncClosableHistory();

    ClosableUiItem item = ClosableUiItem::NodeContextMenu;
    const bool hasOpen = UiCloseStack::PopNextOpen(
        m_ClosableHistory,
        [this](ClosableUiItem candidate) { return IsClosableItemOpen(candidate); },
        item);

    if (!hasOpen) {
        return false;
    }

    CloseClosableItem(item);
    SyncClosableHistory();
    return true;
}
