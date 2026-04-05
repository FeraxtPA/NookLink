#pragma once
#include <vector>
#include <memory>
#include <functional>
#include <string>
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
        std::function<void(std::string, std::string, std::string, float, Status, std::string)> onAddBook,
        std::function<void(int, std::string, std::string, std::string, float, Status, std::string)> onEditBook,
        std::function<void()> onToggleLayout,
        std::function<void(Status)> onToggleStatus
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
        for (auto& w : m_Widgets) {
            w->OnWindowResize(width, height);
        }
    }

private:
    int m_ScreenWidth, m_ScreenHeight;
    std::vector<std::shared_ptr<Widget>> m_Widgets;

    // Tooltip State
    Node* m_LastHoveredNode = nullptr;
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
    std::shared_ptr<Button> m_ApplyFiltersBtn;
    std::string m_ActiveFilterQuery = "";

    bool m_IsLotteryRolling = false;
    float m_LotteryTimer = 0.0f;
    float m_LotterySpeedTimer = 0.0f;

    int m_LotteryWinnerId = -1;
    bool m_LastLotteryCheckState = false;

    std::string m_NotificationText{ "" };
    float m_NotificationTimer = 0.0f;
  
    std::shared_ptr<TextInput> m_SearchBar;
   
    std::shared_ptr<TextBox> m_EditNotes;

    std::shared_ptr<Button> m_EditStatusBtn;
    int m_EditingBookId = -1;
    std::shared_ptr<int> m_EditStatusState;

  

    // Book detail panel
    std::shared_ptr<Panel> m_BookDetailsPanel;

    // Zmìnìno z TextBox na Label:
    std::shared_ptr<Label> m_DetailsText;

    std::shared_ptr<Button> m_DetailsEditBtn;
    std::shared_ptr<Button> m_DetailsCloseBtn;

    Book* m_CurrentDetailsBook = nullptr;

    void UpdateTooltipCache(const GraphManager* graphRenderer, const BookManager& bookManager, TextRenderer* textRenderer);
    void DrawHelpText(TextRenderer* renderer) const;
    void DrawTooltip(Vector2 mousePos, TextRenderer* renderer) const;
    void DrawNotification(TextRenderer* textRenderer) const;
};