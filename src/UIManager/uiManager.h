
// Central UI management class for the NookLink application.
// Handles creation, layout, updating, and rendering of all UI components.
// Manages panels for book management, reading goals, filtering, and search.


#pragma once
#include <vector>
#include <memory>
#include <functional>
#include <string>
#include <array>
#include <raylib.h>

#include "book.h"
#include "nodeRenderer.h"

class BookManager;
class GraphManager;
class TextRenderer;
class Widget;
class Panel;
class TextInput;
class TextBox;
class Button;
class Checkbox;
class Slider;
class Label;
class FlexLayout;
class CalendarWidget;
class Dropdown;

class UIManager {
public:
    enum class NodeContextAction {
        None,
        EditBook,
        DeleteBook,
        ToggleLock
    };

    enum class AnalyticsRightPanelMode {
        StatusPie,
        RatingProgress,
        RatingHistogram,
        ReadingMomentum,
        AuthorDistribution,
        GenreTreemap,
        PageDistribution,
        PublicationTimeline
    };

    struct BulkEditRequest {
        bool removeGenre = false;
        std::string genre;
        bool hasStatus = false;
        Status status = Status::ToRead;
        bool hasPublished = false;
        std::string published;
        bool hasPages = false;
        int pages = 0;
        bool applyOnlyIfEmpty = false;
    };

    using AddBookCallback = std::function<void(std::string, std::string, std::string, int, std::string, float, Status, std::string, std::string, std::string)>;
    using EditBookCallback = std::function<void(int, std::string, std::string, std::string, int, std::string, float, Status, std::string, std::string, std::string)>;

    UIManager(int w, int h);
    ~UIManager();

    bool IsMouseOverUI() const;

    void BuildInterface(
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
        std::function<void(int)> onSortBooks
    );

    void OpenBookDetails(Book* book);
    void OpenGenreDetails(const std::string& genreName, int connectedBooks, const std::vector<std::string>& sampleTitles);
    void OpenEditPanel(Book* book);


    bool IsBlockingGraphInteraction() const { return isBlockingGraph; }

    void Update(BookManager& bookManager, GraphManager* graphRenderer);
    void Draw(Vector2 mousePos, GraphManager* graphRenderer, const BookManager& bookManager, TextRenderer* textRenderer) const;

    std::string GetSearchText() const;

    bool IsSearchBarFocused() const;
    void SetSearchFocusIndicator(int oneBasedIndex, int totalCount);
    void ClearSearchFocusIndicator();

    void FocusSearchBar();
    void SetMultiSelectIndicator(bool isActive, int selectedCount);
    void OpenAddPanel();
    void ToggleSettingsPanel();
    void ToggleSettingsHelpPanel();
    void ToggleFilterPanel();
    void OpenBulkGenreAssignPanel(int selectedCount);
    bool PollBulkEditRequest(BulkEditRequest& outRequest);
    void OpenNodeContextMenu(int nodeId, NodeType nodeType, Vector2 screenPos, bool isLocked);
    void CloseNodeContextMenu();
    bool PollNodeContextAction(NodeContextAction& action, int& nodeId);
    bool HandleEscapeCloseRequest();


    void ShowNotification(const std::string& message, float duration = 2.0f);
    void MarkAnalyticsDirty();

  


    void OnWindowResize(int width, int height);

private:
    enum class ClosableUiItem {
        NodeContextMenu = 0,
        SettingsHelp,
        Calendar,
        BookDetails,
        EditPanel,
        AddPanel,
        SearchFilter,
        Settings,
        Analytics,
        ReadingGoal,
        Lottery,
        BulkGenreAssign,
        Count
    };

    int m_ScreenWidth, m_ScreenHeight;
    std::vector<std::shared_ptr<Widget>> m_Widgets;

    bool isBlockingGraph = false;


    std::shared_ptr<Panel> m_EditPanel;
    std::shared_ptr<TextInput> m_EditTitle;
    std::shared_ptr<TextInput> m_EditAuthor;
    std::shared_ptr<TextInput> m_EditGenres;
    std::shared_ptr<Dropdown> m_EditGenreDropdown;
    std::shared_ptr<TextInput> m_EditPages;
    std::shared_ptr<TextInput> m_EditPublishedDate;
    std::shared_ptr<TextInput> m_EditRating;
    std::shared_ptr<TextInput> m_EditStartedDate;
    std::shared_ptr<TextInput> m_EditFinishedDate;
    std::shared_ptr<Panel> m_AddPanel;
    std::shared_ptr<TextInput> m_AddIsbn;
    std::shared_ptr<Button> m_AddFetchIsbnBtn;
    std::shared_ptr<TextInput> m_AddTitle;
    std::shared_ptr<TextInput> m_AddAuthor;
    std::shared_ptr<TextInput> m_AddGenres;
    std::shared_ptr<Dropdown> m_AddGenreDropdown;
    std::shared_ptr<TextInput> m_AddPages;
    std::shared_ptr<TextInput> m_AddPublishedDate;
    std::shared_ptr<TextInput> m_AddRating;
    std::shared_ptr<TextInput> m_AddStartedDate;
    std::shared_ptr<TextInput> m_AddFinishedDate;
    std::shared_ptr<TextBox> m_AddNotes;
    std::shared_ptr<Button> m_AddStatusBtn;
    std::shared_ptr<int> m_AddStatusState;

    std::shared_ptr<Panel> m_LotteryPanel;
    std::shared_ptr<TextBox> m_LotteryText;
    std::shared_ptr<Button> m_LotteryCloseBtn;
    std::shared_ptr<Checkbox> m_LotteryAutoRead;

    std::shared_ptr<Panel> m_BulkGenreAssignPanel;
    std::shared_ptr<TextInput> m_BulkGenreAssignInput;
    std::shared_ptr<TextInput> m_BulkGenreAssignPublishedInput;
    std::shared_ptr<TextInput> m_BulkGenreAssignPagesInput;
    std::shared_ptr<Button> m_BulkGenreAssignApplyBtn;
    std::shared_ptr<Button> m_BulkGenreAssignRemoveBtn;
    std::shared_ptr<Button> m_BulkGenreAssignStatusBtn;
    std::shared_ptr<Button> m_BulkGenreAssignCancelBtn;
    std::shared_ptr<Checkbox> m_BulkGenreAssignOnlyIfEmpty;
    std::shared_ptr<Label> m_BulkGenreAssignInfoLabel;
    std::shared_ptr<int> m_BulkGenreAssignStatusState;
    bool m_HasPendingBulkEditRequest = false;
    BulkEditRequest m_PendingBulkEditRequest{};

    //Filer Menu
    std::shared_ptr<Panel> m_SearchFilterPanel;
    std::shared_ptr<Checkbox> m_CheckToRead;
    std::shared_ptr<Checkbox> m_CheckReading;
    std::shared_ptr<Checkbox> m_CheckRead;
    std::shared_ptr<Label> m_FilterRatingLabel;
    std::shared_ptr<Slider> m_FilterRatingSlider;
    std::shared_ptr<Label> m_FilterGenreLabel;
    std::shared_ptr<TextInput> m_FilterGenreInput;
    std::shared_ptr<Button> m_FilterFinishedRangeBtn;
    std::shared_ptr<Button> m_FilterSortBtn;
    std::shared_ptr<Button> m_ApplyFiltersBtn;
    std::string m_ActiveFilterQuery = "";
    int m_FilterFinishedRangeState = 0;
    int m_FilterSortState = 0;

    bool m_IsLotteryRolling = false;
    float m_LotteryTimer = 0.0f;
    float m_LotterySpeedTimer = 0.0f;

    int m_LotteryWinnerId = -1;
    bool m_LastLotteryCheckState = false;

    std::string m_NotificationText{ "" };
    float m_NotificationTimer = 0.0f;

    bool m_MultiSelectIndicatorActive = false;
    int m_MultiSelectIndicatorCount = 0;
  
    std::shared_ptr<TextInput> m_SearchBar;
    int m_SearchFocusIndicatorIndex = -1;
    int m_SearchFocusIndicatorTotal = 0;
    std::shared_ptr<FlexLayout> m_ToolbarLayout;
    std::shared_ptr<Button> m_ToggleFiltersBtn;
    std::shared_ptr<Button> m_OpenStatsBtn;
    std::shared_ptr<Button> m_OpenSettingsBtn;
    std::shared_ptr<Panel> m_SettingsPanel;
    std::shared_ptr<Dropdown> m_ThemeDropdown;
    std::shared_ptr<Slider> m_LayoutDensitySlider;
    std::shared_ptr<Label> m_LayoutDensityValueLabel;
    std::shared_ptr<Button> m_SettingsHelpToggleBtn;
    bool m_SettingsHelpExpanded = false;
    std::vector<std::string> m_KnownGenres;

    std::shared_ptr<Panel> m_AnalyticsPanel;
    std::shared_ptr<Label> m_AnalyticsOverviewLabel;
    std::shared_ptr<Label> m_AnalyticsTopGenresListLabel;
    std::shared_ptr<Label> m_AnalyticsTopAuthorsListLabel;
    std::shared_ptr<Label> m_AnalyticsTopRatedListLabel;
    std::shared_ptr<Label> m_AnalyticsPagesLabel;
    std::shared_ptr<Label> m_AnalyticsPublicationLabel;
    std::shared_ptr<Button> m_AnalyticsChartModePrevBtn;
    std::shared_ptr<Button> m_AnalyticsChartModeBtn;
    std::shared_ptr<Button> m_AnalyticsPageBinsBtn;
    bool m_AnalyticsDirty = true;
    float m_AnalyticsRefreshTimer = 0.0f;
    int m_AnalyticsTotalBooks = 0;
    int m_AnalyticsToReadCount = 0;
    int m_AnalyticsReadingCount = 0;
    int m_AnalyticsReadCount = 0;
    int m_AnalyticsRatedCount = 0;
    float m_AnalyticsAvgRating = 0.0f;
    int m_AnalyticsFinishedThisMonth = 0;
    int m_AnalyticsFinishedThisYear = 0;
    std::vector<std::pair<std::string, int>> m_AnalyticsTopGenres;
    std::vector<std::pair<std::string, int>> m_AnalyticsTopAuthors;
    std::vector<std::pair<std::string, float>> m_AnalyticsTopRatedBooks;
    std::array<int, 5> m_AnalyticsRatingBins{};
    int m_AnalyticsBooksWithPages = 0;
    float m_AnalyticsAvgPages = 0.0f;
    int m_AnalyticsPageBinCount = 8;
    std::vector<int> m_AnalyticsPageBins;
    std::vector<std::pair<int, int>> m_AnalyticsPageBinRanges;
    int m_AnalyticsPublishedYearCount = 0;
    int m_AnalyticsOldestPublishedYear = 0;
    int m_AnalyticsNewestPublishedYear = 0;
    std::vector<std::pair<int, int>> m_AnalyticsPublishedByDecade;
    AnalyticsRightPanelMode m_AnalyticsRightPanelMode = AnalyticsRightPanelMode::StatusPie;

    std::shared_ptr<Panel> m_ReadingGoalPanel;
    std::shared_ptr<Label> m_GoalSummaryLabel;
    std::shared_ptr<Label> m_GoalProgressLabel;

    std::function<int()> m_GetReadCount;
    std::function<int()> m_GetGoalTarget;
    std::function<int()> m_GetGoalProgress;
    std::function<void(int)> m_OnAdjustGoalTarget;
    std::function<void()> m_OnResetGoalProgress;
   
    std::shared_ptr<TextBox> m_EditNotes;

    std::shared_ptr<Button> m_EditStatusBtn;
    int m_EditingBookId = -1;
    std::shared_ptr<int> m_EditStatusState;

  

    
    std::shared_ptr<Panel> m_BookDetailsPanel;

   
    std::shared_ptr<Label> m_DetailsText;

    std::shared_ptr<Button> m_DetailsEditBtn;
    std::shared_ptr<Button> m_DetailsCloseBtn;

    Book m_CurrentDetailsBook{};
    bool m_HasCurrentDetailsBook = false;

    void RefreshKnownGenresFromBookManager(const BookManager& bookManager);
    void SyncGenreDropdownOptions();
    void DrawHelpText(TextRenderer* renderer) const;
    void DrawAnalyticsCharts(TextRenderer* renderer) const;
    void LayoutAnalyticsLeftLabels() const;
    Rectangle GetSettingsHelpRect() const;
    void DrawNotification(TextRenderer* textRenderer) const;
    void UpdateGoalPanelTexts();
    void UpdateAnalyticsPanelTexts(const BookManager& bookManager);
    void OpenCalendarFor(const std::shared_ptr<TextInput>& targetInput);
    std::shared_ptr<FlexLayout> CreateButtonRow(Vector2 size, float gap = 12.0f);
    std::shared_ptr<FlexLayout> CreateDateInputRow(const std::shared_ptr<TextInput>& input);

    void BuildFilterPanel(const std::function<void(Status)>& onToggleStatus, const std::function<void(int)>& onSortBooks);
    void BuildToolbar(
        const std::function<void()>& onSave,
        const std::function<void()>& onSaveAs,
        const std::function<void()>& onLoad,
        const std::function<void()>& onBackToMenu,
        const std::function<void()>& onUndo,
        const std::function<void()>& onRedo,
        const std::function<void()>& onToggleLayout);
    void BuildSettingsPanel(
        const std::function<std::string(int)>& onSelectTheme,
        const std::function<int()>& getCurrentThemeIndex,
        const std::function<int()>& getThemeCount,
        const std::function<std::string(int)>& getThemeNameByIndex,
        const std::function<void(float)>& onSetLayoutDensity,
        const std::function<float()>& getLayoutDensity,
        const std::function<void()>& onImportCsv,
        const std::function<void()>& onExportCsv);
    void BuildBookDetailsPanel();
    void BuildAddPanel(const AddBookCallback& onAddBook);
    void BuildEditPanel(const EditBookCallback& onEditBook);
    void BuildLotteryPanel();
    void BuildBulkGenreAssignPanel();
    void BuildReadingGoalPanel();
    void BuildAnalyticsPanel();

    std::shared_ptr<CalendarWidget> m_CalendarWidget;
    std::shared_ptr<TextInput> m_ActiveDateInput;

    void TrackClosableItem(ClosableUiItem item, bool isOpen);
    void SyncClosableHistory();
    bool IsClosableItemOpen(ClosableUiItem item) const;
    void CloseClosableItem(ClosableUiItem item);
    std::array<bool, (size_t)ClosableUiItem::Count> m_LastClosableOpenStates{};
    std::vector<ClosableUiItem> m_ClosableHistory{};

    bool m_NodeContextMenuVisible = false;
    int m_NodeContextNodeId = -1;
    NodeType m_NodeContextNodeType = NodeType::Book;
    bool m_NodeContextNodeLocked = false;
    Rectangle m_NodeContextMenuBounds{ 0, 0, 0, 0 };
    int m_NodeContextHoverIndex = -1;
    NodeContextAction m_PendingNodeContextAction = NodeContextAction::None;
    int m_PendingNodeContextNodeId = -1;
};