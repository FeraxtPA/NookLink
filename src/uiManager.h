#pragma once
#include <vector>
#include <memory>
#include <functional>
#include <string>
#include <raylib.h>

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
        std::function<void(std::string title, std::string author, std::string genres, float rating, Status status, std::string notes)> onAddBook,
        std::function<void(int id, std::string t, std::string a, std::string g, float r, Status s, std::string n)> onEditBook);

    void OpenEditPanel(Book* book);

    void Update(Vector2 worldMousePos, Vector2 mousePos, BookManager& bookManager, GraphManager* graphRenderer, TextRenderer* textRenderer);
    void Draw(Vector2 mousePos, GraphManager* graphRenderer, const BookManager& bookManager, TextRenderer* textRenderer) const;

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

    
    std::shared_ptr<Panel> m_EditPanel;
    std::shared_ptr<TextInput> m_EditTitle;
    std::shared_ptr<TextInput> m_EditAuthor;
    std::shared_ptr<TextInput> m_EditGenres;
    std::shared_ptr<TextInput> m_EditRating;

    std::shared_ptr<Panel> m_LotteryPanel;
    std::shared_ptr<TextBox> m_LotteryText;
    std::shared_ptr<Button> m_LotteryCloseBtn;
    std::shared_ptr<Checkbox> m_LotteryAutoRead;


    bool m_IsLotteryRolling = false;
    float m_LotteryTimer = 0.0f;
    float m_LotterySpeedTimer = 0.0f;

    int m_LotteryWinnerId = -1;
    bool m_LastLotteryCheckState = false;

  
    std::shared_ptr<TextInput> m_SearchBar;
   
    std::shared_ptr<TextBox> m_EditNotes;

    std::shared_ptr<Button> m_EditStatusBtn;
    int m_EditingBookId = -1;
    std::shared_ptr<int> m_EditStatusState;

    std::shared_ptr<Panel> m_FilterPanel;
    void RebuildFilterPanel(GraphManager* graphManager);

    void UpdateTooltipCache(const GraphManager* graphRenderer, const BookManager& bookManager, TextRenderer* textRenderer);
    void DrawHelpText(TextRenderer* renderer) const;
    void DrawTooltip(Vector2 mousePos, TextRenderer* renderer) const;
};