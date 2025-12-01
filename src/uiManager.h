#pragma once

#include <raylib.h>
#include <string>
#include <vector>
#include <memory> 
#include "bookManager.h" 
#include "graphManager.h"
#include "colors.h" 
#include "book.h" 
#include "UI/widget.h"
#include "UI/button.h"



class UIManager
{
public:
    
    //Should only use font from textRenderer not creating multiple fonts...
    UIManager(int screenWidth, int screenHeight);
    ~UIManager();

  
    void Update(Vector2 worldMousePos, Vector2 mousePos, BookManager& bookManager, GraphManager* graphRenderer);

   
    void Draw(Vector2 mousePos,  GraphManager* graphRenderer, const BookManager& bookManager) const;

    void BuildInterface(
        std::function<void()> onSave,
        std::function<void()> onLoad
    );

private:
  
    
    

    const int screenWidth;
    const int screenHeight;
    std::vector<std::shared_ptr<Widget>> m_Widgets;


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
    void DrawHelpText() const;
    void DrawTooltip(Vector2 mousePos) const;


    
};