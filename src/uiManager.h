#pragma once
#include <vector>
#include <memory>
#include <functional>
#include <string>
#include <algorithm>
#include <raylib.h>
#include <format>

#include "UI/widget.h"
#include "bookManager.h"
#include "graphManager.h"
#include "textRenderer.h" 

// UI Components
#include "UI/panel.h"
#include "UI/textInput.h"
#include "UI/textBox.h"  
#include "UI/button.h"
#include "UI/checkbox.h"
#include "UI/slider.h"
#include "UI/label.h"
#include "UI/flexLayout.h"
#include "UI/calendarWidget.h"

class UIManager {
public:
    UIManager(int w, int h);
    ~UIManager();

    bool IsMouseOverUI() const;

    void BuildInterface(
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
        std::function<void(int)> onSortBooks
    );

    void OpenBookDetails(Book* book);
    void OpenEditPanel(Book* book);


    bool IsBlockingGraphInteraction() const { return isBlockingGraph; }

    void Update(Vector2 worldMousePos, Vector2 mousePos, BookManager& bookManager, GraphManager* graphRenderer, TextRenderer* textRenderer);
    void Draw(Vector2 mousePos, GraphManager* graphRenderer, const BookManager& bookManager, TextRenderer* textRenderer) const;


    void ShowNotification(const std::string& message, float duration = 2.0f);

  


    void OnWindowResize(int width, int height) {
        m_ScreenWidth = width;
        m_ScreenHeight = height;
        if (m_ToolbarLayout) {
            m_ToolbarLayout->SetSize({ std::max(0.0f, (float)width - kToolbarWidthInset), kToolbarRowHeight });
        }
        for (auto& w : m_Widgets) {
            w->OnWindowResize(width, height);
        }
    }

private:
    static constexpr float kToolbarWidthInset = 52.0f;
    static constexpr float kToolbarRowHeight = 40.0f;

    int m_ScreenWidth, m_ScreenHeight;
    std::vector<std::shared_ptr<Widget>> m_Widgets;

    // Tooltip State
    int m_LastHoveredNodeId = -1;
    NodeType m_LastHoveredNodeType = NodeType::Book;
    double m_HoverStartTime = 0.0;
    mutable std::string m_CachedTooltipText;
    mutable std::vector<std::string> m_CachedLines;
    mutable int m_CachedBoxWidth = 0;
    mutable int m_CachedBoxHeight = 0;
    Vector2 m_LastMousePos = { -1, -1 };

    
    bool isBlockingGraph = false;


    std::shared_ptr<Panel> m_EditPanel;
    std::shared_ptr<TextInput> m_EditTitle;
    std::shared_ptr<TextInput> m_EditAuthor;
    std::shared_ptr<TextInput> m_EditGenres;
    std::shared_ptr<TextInput> m_EditRating;
    std::shared_ptr<TextInput> m_EditStartedDate;
    std::shared_ptr<TextInput> m_EditFinishedDate;
    std::shared_ptr<Panel> m_AddPanel;
    std::shared_ptr<TextInput> m_AddTitle;
    std::shared_ptr<TextInput> m_AddAuthor;
    std::shared_ptr<TextInput> m_AddGenres;
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
  
    std::shared_ptr<TextInput> m_SearchBar;
    std::shared_ptr<FlexLayout> m_ToolbarLayout;

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

  

    // Book detail panel
    std::shared_ptr<Panel> m_BookDetailsPanel;

    // Zm�n�no z TextBox na Label:
    std::shared_ptr<Label> m_DetailsText;

    std::shared_ptr<Button> m_DetailsEditBtn;
    std::shared_ptr<Button> m_DetailsCloseBtn;

    Book m_CurrentDetailsBook{};
    bool m_HasCurrentDetailsBook = false;

    void UpdateTooltipCache(const GraphManager* graphRenderer, const BookManager& bookManager, TextRenderer* textRenderer);
    void DrawHelpText(TextRenderer* renderer) const;
    void DrawTooltip(Vector2 mousePos, TextRenderer* renderer) const;
    void DrawNotification(TextRenderer* textRenderer) const;
    void UpdateGoalPanelTexts();
    void OpenCalendarFor(const std::shared_ptr<TextInput>& targetInput);

    std::shared_ptr<CalendarWidget> m_CalendarWidget;
    std::shared_ptr<TextInput> m_ActiveDateInput;
};