#include "uiManager.h"

#include "bookManager.h"
#include "graphManager.h"
#include "UI/button.h"
#include "UI/calendarWidget.h"
#include "UI/checkbox.h"
#include "UI/dropdown.h"
#include "UI/label.h"
#include "UI/panel.h"
#include "UI/textBox.h"
#include "UI/textInput.h"
#include "UI/widget.h"

#include <algorithm>
#include <format>

void UIManager::FocusSearchBar()
{
    auto blurInput = [](const std::shared_ptr<TextInput>& input) {
        if (input) input->isFocused = false;
    };
    auto blurBox = [](const std::shared_ptr<TextBox>& box) {
        if (box) box->SetFocused(false);
    };

    blurInput(m_AddTitle);
    blurInput(m_AddAuthor);
    blurInput(m_AddGenres);
    blurInput(m_AddIsbn);
    blurInput(m_AddPages);
    blurInput(m_AddPublishedDate);
    blurInput(m_AddRating);
    blurInput(m_AddStartedDate);
    blurInput(m_AddFinishedDate);

    blurInput(m_EditTitle);
    blurInput(m_EditAuthor);
    blurInput(m_EditGenres);
    blurInput(m_EditPages);
    blurInput(m_EditPublishedDate);
    blurInput(m_EditRating);
    blurInput(m_EditStartedDate);
    blurInput(m_EditFinishedDate);

    blurInput(m_FilterGenreInput);
    blurInput(m_BulkGenreAssignInput);
    blurInput(m_BulkGenreAssignPublishedInput);
    blurInput(m_BulkGenreAssignPagesInput);
    blurBox(m_AddNotes);
    blurBox(m_EditNotes);

    if (m_SearchBar) {
        m_SearchBar->isFocused = true;
    }
}

void UIManager::SetMultiSelectIndicator(bool isActive, int selectedCount)
{
    m_MultiSelectIndicatorActive = isActive;
    m_MultiSelectIndicatorCount = std::max(0, selectedCount);
}

void UIManager::OpenBulkGenreAssignPanel(int selectedCount)
{
    if (!m_BulkGenreAssignPanel || !m_BulkGenreAssignInput || !m_BulkGenreAssignInfoLabel) {
        return;
    }

    m_BulkGenreAssignInfoLabel->SetText("Selected books: " + std::to_string(std::max(0, selectedCount)));
    m_BulkGenreAssignInput->Clear();
    if (m_BulkGenreAssignPublishedInput) m_BulkGenreAssignPublishedInput->Clear();
    if (m_BulkGenreAssignPagesInput) m_BulkGenreAssignPagesInput->Clear();
    if (m_BulkGenreAssignOnlyIfEmpty) m_BulkGenreAssignOnlyIfEmpty->checked = false;
    if (m_BulkGenreAssignStatusState) *m_BulkGenreAssignStatusState = 0;
    if (m_BulkGenreAssignStatusBtn) m_BulkGenreAssignStatusBtn->SetText("Status: (unchanged)");
    m_BulkGenreAssignInput->isFocused = true;
    m_BulkGenreAssignPanel->SetVisible(true);
}

bool UIManager::PollBulkEditRequest(BulkEditRequest& outRequest)
{
    if (!m_HasPendingBulkEditRequest) {
        return false;
    }

    outRequest = m_PendingBulkEditRequest;
    m_PendingBulkEditRequest = BulkEditRequest{};
    m_HasPendingBulkEditRequest = false;
    return true;
}

void UIManager::OpenAddPanel()
{
    if (m_AddPanel) {
        m_AddPanel->SetVisible(true);
    }
}

void UIManager::ToggleSettingsPanel()
{
    if (!m_SettingsPanel) {
        return;
    }

    m_SettingsPanel->SetVisible(!m_SettingsPanel->IsVisible());
    if (!m_SettingsPanel->IsVisible()) {
        m_SettingsHelpExpanded = false;
        if (m_SettingsHelpToggleBtn) {
            m_SettingsHelpToggleBtn->SetText("? Controls Help");
        }
    }
}

void UIManager::ToggleSettingsHelpPanel()
{
    if (!m_SettingsPanel) {
        return;
    }

    if (!m_SettingsPanel->IsVisible()) {
        m_SettingsPanel->SetVisible(true);
    }

    m_SettingsHelpExpanded = !m_SettingsHelpExpanded;
    if (m_SettingsHelpToggleBtn) {
        m_SettingsHelpToggleBtn->SetText(m_SettingsHelpExpanded ? "? Hide Controls Help" : "? Controls Help");
    }
}

void UIManager::ToggleFilterPanel()
{
    if (m_SearchFilterPanel) {
        m_SearchFilterPanel->SetVisible(!m_SearchFilterPanel->IsVisible());
    }
}

void UIManager::OpenNodeContextMenu(int nodeId, NodeType nodeType, Vector2 screenPos, bool isLocked)
{
    m_NodeContextNodeId = nodeId;
    m_NodeContextNodeType = nodeType;
    m_NodeContextNodeLocked = isLocked;
    m_NodeContextHoverIndex = -1;

    const float menuWidth = NookConst::Layout::kContextMenuWidth;
    float menuHeight = NookConst::Layout::kContextMenuHeaderHeight;
    if (nodeType == NodeType::Book) {
        menuHeight = NookConst::Layout::kContextMenuHeaderHeight + 3.0f * NookConst::Layout::kContextMenuItemHeight;
    }
    else {
        menuHeight = NookConst::Layout::kContextMenuHeaderHeight + 1.0f * NookConst::Layout::kContextMenuItemHeight;
    }

    float menuX = screenPos.x + NookConst::Layout::kContextMenuOffset;
    float menuY = screenPos.y + NookConst::Layout::kContextMenuOffset;
    menuX = std::clamp(menuX, NookConst::Layout::kContextMenuScreenPadding, std::max(NookConst::Layout::kContextMenuScreenPadding, (float)m_ScreenWidth - menuWidth - NookConst::Layout::kContextMenuScreenPadding));
    menuY = std::clamp(menuY, NookConst::Layout::kContextMenuScreenPadding, std::max(NookConst::Layout::kContextMenuScreenPadding, (float)m_ScreenHeight - menuHeight - NookConst::Layout::kContextMenuScreenPadding));

    m_NodeContextMenuBounds = { menuX, menuY, menuWidth, menuHeight };
    m_NodeContextMenuVisible = true;
}

void UIManager::CloseNodeContextMenu()
{
    m_NodeContextMenuVisible = false;
    m_NodeContextNodeId = -1;
    m_NodeContextHoverIndex = -1;
}

bool UIManager::PollNodeContextAction(NodeContextAction& action, int& nodeId)
{
    if (m_PendingNodeContextAction == NodeContextAction::None || m_PendingNodeContextNodeId < 0) {
        return false;
    }

    action = m_PendingNodeContextAction;
    nodeId = m_PendingNodeContextNodeId;
    m_PendingNodeContextAction = NodeContextAction::None;
    m_PendingNodeContextNodeId = -1;
    return true;
}

void UIManager::OpenCalendarFor(const std::shared_ptr<TextInput>& targetInput)
{
    if (!targetInput || !m_CalendarWidget) {
        return;
    }

    // Keep a weak editing context so date callbacks know which field to write into.
    m_ActiveDateInput = targetInput;

    const Rectangle inputBounds = targetInput->GetBounds();
    const Vector2 preferredTopLeft = {
        inputBounds.x,
        inputBounds.y + inputBounds.height + 8.0f
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

void UIManager::OpenEditPanel(Book* book)
{
    if (!book || !m_EditPanel) return;

    m_EditingBookId = book->getId();

    m_EditTitle->text = book->getTitle();
    m_EditAuthor->text = book->getAuthor();
    m_EditPages->text = book->getPageCount() > 0 ? std::to_string(book->getPageCount()) : "";
    m_EditPublishedDate->text = book->getDatePublished();

    m_EditRating->text = std::format("{:.2f}", book->getRating());
    m_EditNotes->SetText(book->getNotes());

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

    *m_EditStatusState = (int)book->getStatus();
    if (*m_EditStatusState == 0) m_EditStatusBtn->SetText("Status: To Read");
    else if (*m_EditStatusState == 1) m_EditStatusBtn->SetText("Status: Reading");
    else m_EditStatusBtn->SetText("Status: Read");

    m_EditPanel->SetVisible(true);
}

void UIManager::Update(BookManager& bookManager, GraphManager* graphRenderer)
{
    Widget::ResetInputConsumption();
    RefreshKnownGenresFromBookManager(bookManager);

    if (m_AnalyticsPanel && m_AnalyticsPanel->IsVisible()) {
        m_AnalyticsRefreshTimer -= GetFrameTime();
        if (m_AnalyticsDirty || m_AnalyticsRefreshTimer <= 0.0f) {
            UpdateAnalyticsPanelTexts(bookManager);
            m_AnalyticsDirty = false;
            m_AnalyticsRefreshTimer = NookConst::Timing::kAnalyticsRefreshInterval;
        }
    }

    if (m_AnalyticsPageBinsBtn) {
        m_AnalyticsPageBinsBtn->SetVisible(
            m_AnalyticsPanel &&
            m_AnalyticsPanel->IsVisible() &&
            m_AnalyticsRightPanelMode == AnalyticsRightPanelMode::PageDistribution);
    }

    if (m_NotificationTimer > 0.0f) {
        m_NotificationTimer -= GetFrameTime();
    }

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
        m_ToggleFiltersBtn->SetText(hasAdvancedFilters ? NookConst::Text::kFilterToolbarGlyphActive : NookConst::Text::kFilterToolbarGlyph);
    }

    UpdateGoalPanelTexts();

    if (m_IsLotteryRolling)
    {
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
                    MarkAnalyticsDirty();
                }

                std::string statusMsg = m_LotteryAutoRead->checked ? "\n(Status updated to Reading!)" : "";
                m_LotteryText->SetText(
                    "WINNER!\n\n" +
                    winnerConst.getTitle() + "\n" +
                    "by " + winnerConst.getAuthor() + "\n" +
                    statusMsg
                );
            }
            catch (const std::exception&) {
                m_LotteryText->SetText("No books found with status 'To Read'!");
                m_LotteryWinnerId = -1;
            }
            m_LotteryCloseBtn->SetVisible(true);
        }
    }
    else if (m_LotteryPanel->IsVisible() && m_LotteryWinnerId != -1)
    {
        if (m_LotteryAutoRead->checked != m_LastLotteryCheckState)
        {
            m_LastLotteryCheckState = m_LotteryAutoRead->checked;

            Book* winner = bookManager.getBookById(m_LotteryWinnerId);
            if (winner) {
                if (m_LotteryAutoRead->checked) {
                    winner->setStatus(Status::Reading);
                }
                else {
                    winner->setStatus(Status::ToRead);
                }

                if (graphRenderer) graphRenderer->initializePositions();
                MarkAnalyticsDirty();

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

    for (auto it = m_Widgets.rbegin(); it != m_Widgets.rend(); ++it) {
        (*it)->BeginFrameInputRecursive();
        (*it)->Update();
    }

    MouseCursor resolvedCursor = MOUSE_CURSOR_DEFAULT;
    for (auto it = m_Widgets.rbegin(); it != m_Widgets.rend(); ++it) {
        const MouseCursor requested = (*it)->ResolveRequestedCursorRecursive();
        if (requested != MOUSE_CURSOR_DEFAULT) {
            resolvedCursor = requested;
            break;
        }
    }

    auto collapseSettingsHelp = [this]() {
        m_SettingsHelpExpanded = false;
        if (m_SettingsHelpToggleBtn) {
            m_SettingsHelpToggleBtn->SetText("? Controls Help");
        }
    };

    if (m_SettingsPanel && !m_SettingsPanel->IsVisible() && m_SettingsHelpExpanded) {
        collapseSettingsHelp();
    }

    if (m_SettingsHelpExpanded && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        const Vector2 mouse = GetMousePosition();
        const Rectangle helpRect = GetSettingsHelpRect();
        const bool clickedHelp = helpRect.width > 0.0f && helpRect.height > 0.0f && CheckCollisionPointRec(mouse, helpRect);
        const bool clickedHelpToggle = m_SettingsHelpToggleBtn && m_SettingsHelpToggleBtn->IsVisible() && CheckCollisionPointRec(mouse, m_SettingsHelpToggleBtn->GetBounds());

        if (!clickedHelp && !clickedHelpToggle) {
            collapseSettingsHelp();
        }
    }

    if (m_CalendarWidget && !m_CalendarWidget->IsOpen()) {
        m_ActiveDateInput.reset();
    }

    SyncClosableHistory();

    if (m_NodeContextMenuVisible) {
        const Vector2 mouse = GetMousePosition();
        const Rectangle listRect = {
            m_NodeContextMenuBounds.x + 6.0f,
            m_NodeContextMenuBounds.y + 34.0f,
            m_NodeContextMenuBounds.width - 12.0f,
            m_NodeContextMenuBounds.height - 40.0f
        };

        const int itemCount = (m_NodeContextNodeType == NodeType::Book) ? 3 : 1;
        m_NodeContextHoverIndex = -1;
        if (CheckCollisionPointRec(mouse, listRect)) {
            const float itemHeight = listRect.height / (float)itemCount;
            const int idx = (int)((mouse.y - listRect.y) / itemHeight);
            if (idx >= 0 && idx < itemCount) {
                m_NodeContextHoverIndex = idx;
            }
        }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (m_NodeContextHoverIndex >= 0) {
                NodeContextAction chosen = NodeContextAction::None;
                if (m_NodeContextNodeType == NodeType::Book) {
                    if (m_NodeContextHoverIndex == 0) chosen = NodeContextAction::EditBook;
                    else if (m_NodeContextHoverIndex == 1) chosen = NodeContextAction::DeleteBook;
                    else if (m_NodeContextHoverIndex == 2) chosen = NodeContextAction::ToggleLock;
                }
                else {
                    chosen = NodeContextAction::ToggleLock;
                }

                if (chosen != NodeContextAction::None) {
                    m_PendingNodeContextAction = chosen;
                    m_PendingNodeContextNodeId = m_NodeContextNodeId;
                }

                CloseNodeContextMenu();
                Widget::ConsumeLeftClick();
            }
            else if (!CheckCollisionPointRec(mouse, m_NodeContextMenuBounds)) {
                CloseNodeContextMenu();
            }
        }
    }

    isBlockingGraph = hoverBeforeUpdate || IsMouseOverUI() || m_NodeContextMenuVisible;

    SetMouseCursor(resolvedCursor);
}
