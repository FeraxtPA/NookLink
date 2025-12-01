#pragma once

#include <raylib.h>
#include <string>
#include <vector>
#include <memory> 
#include "bookManager.h" 
#include "graphManager.h"
#include "colors.h" 
#include "book.h" 


struct UI_Constants {
    const int screenWidth;
    const int screenHeight;
    // File name for saving/loading book data, user will choose their own file in future
    const char* saveFileName = "my_books.json";

    //Should be relative to screen size later so it works on different resolutions
    const Rectangle saveButton = { (float)screenWidth - 220, 10, 100, 40 };
    const Rectangle loadButton = { (float)screenWidth - 110, 10, 100, 40 };

    UI_Constants(int width, int height) : screenWidth(width), screenHeight(height) {}
};


class UIManager
{
public:
    
    //Should only use font from textRenderer not creating multiple fonts...
    UIManager(int screenWidth, int screenHeight, Font font);
    ~UIManager();

  
    bool Update(Vector2 worldMousePos, Vector2 mousePos, BookManager& bookManager, GraphManager* graphRenderer);

   
    void Draw(Vector2 mousePos,  GraphManager* graphRenderer, const BookManager& bookManager) const;

private:
    UI_Constants m_const;
    Font m_font;

    // --- Stav Tooltipu (pøesunuto z Application) ---
    Node* m_LastHoveredNode = nullptr;
    double m_HoverStartTime = 0.0;
    std::string m_CachedTooltipText;
    std::vector<std::string> m_CachedLines;
    int m_CachedBoxWidth = 0;
    int m_CachedBoxHeight = 0;
    Vector2 m_LastMousePos = { -1, -1 };

    // --- Interní logika ---
    void UpdateTooltipCache(const GraphManager* graphRenderer, const BookManager& bookManager);
    void DrawButtons(Vector2 mousePos) const;
    void DrawHelpText() const;
    void DrawTooltip(Vector2 mousePos) const;

    void DrawButton(const std::string& text) const;
};