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

class UIManager {
public:
    UIManager(int w, int h);
    ~UIManager();


    bool IsMouseOverUI() const;

    void BuildInterface(
        std::function<void()> onSave,
        std::function<void()> onLoad,
        std::function<void(std::string title, std::string author, std::string genres, float rating, Status status, std::string notes)> onAddBook);

    // Update needs renderer for measuring tooltip text
    void Update(Vector2 worldMousePos, Vector2 mousePos, BookManager& bookManager, GraphManager* graphRenderer, TextRenderer* textRenderer);

    // Draw needs renderer for drawing text
    void Draw(Vector2 mousePos, GraphManager* graphRenderer, const BookManager& bookManager, TextRenderer* textRenderer) const;

private:
    int m_ScreenWidth, m_ScreenHeight;
    std::vector<std::shared_ptr<Widget>> m_Widgets;

    // Tooltip State
    Node* m_LastHoveredNode = nullptr;
    double m_HoverStartTime = 0.0;

    // Mutable strings for caching text layout
    mutable std::string m_CachedTooltipText;
    mutable std::vector<std::string> m_CachedLines;
    mutable int m_CachedBoxWidth = 0;
    mutable int m_CachedBoxHeight = 0;
    Vector2 m_LastMousePos = { -1, -1 };

    void UpdateTooltipCache(const GraphManager* graphRenderer, const BookManager& bookManager, TextRenderer* textRenderer);
    void DrawHelpText(TextRenderer* renderer) const;
    void DrawTooltip(Vector2 mousePos, TextRenderer* renderer) const;
};