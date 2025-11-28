#include "UIManager.h"
#include <iostream>
#include <algorithm>



UIManager::UIManager(int screenWidth, int screenHeight, Font font)
    : m_const(screenWidth, screenHeight), m_font(font)
{
 
}

UIManager::~UIManager()
{
}

bool UIManager::Update(Vector2 worldMousePos, Vector2 mousePos, BookManager& bookManager, GraphManager* graphRenderer)
{
    bool layoutChanged = false;

  
    if (!IsMouseButtonDown(MOUSE_RIGHT_BUTTON) && (graphRenderer == nullptr || graphRenderer->getDraggedNode() == nullptr))
    {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            if (CheckCollisionPointRec(mousePos, m_const.saveButton))
            {
                std::cout << "Saving books to " << m_const.saveFileName << "..." << std::endl;
                bookManager.saveBooksToFile(m_const.saveFileName);
            }
            else if (CheckCollisionPointRec(mousePos, m_const.loadButton))
            {
                std::cout << "Loading books from " << m_const.saveFileName << "..." << std::endl;
                bookManager.loadBooksFromFile(m_const.saveFileName);
                graphRenderer->clearGenresAndConnections();
                layoutChanged = true;
            }
        }
    }

  
    bool mouseMoved = (mousePos.x != m_LastMousePos.x || mousePos.y != m_LastMousePos.y);
    m_LastMousePos = mousePos;

    if (graphRenderer != nullptr) {
        Node* newlyHovered = graphRenderer->getNodeAtPosition(worldMousePos);

        if (mouseMoved || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            if (newlyHovered != m_LastHoveredNode || IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
                m_LastHoveredNode = newlyHovered;
                m_HoverStartTime = GetTime();
                m_CachedTooltipText.clear(); // Vynutí pøestavbu pøi zmìnì uzlu
            }
        }
    }

    UpdateTooltipCache(graphRenderer, bookManager);

    return layoutChanged;
}


void UIManager::UpdateTooltipCache(const GraphManager* graphRenderer, const BookManager& bookManager)
{
    if (m_LastHoveredNode == nullptr || m_CachedTooltipText.empty() == false) return;

    if (m_LastHoveredNode->type == NodeType::Book) {
        const Book* hoveredBook = bookManager.findBookById(m_LastHoveredNode->id);
        if (hoveredBook) {
            m_CachedTooltipText = "Title: " + hoveredBook->getTitle() +
                "\nAuthor: " + hoveredBook->getAuthor() +
                "\nCurrent Status: " + statusToString(hoveredBook->getStatus()) +
                "\nNotes: " + hoveredBook->getNotes() +
                "\nRating: " + Book::ratingToStars(hoveredBook->getRating()) +
                "\nBookID: " + std::to_string(hoveredBook->getId());
        }
        else {
            m_CachedTooltipText = "Book not found!";
            m_LastHoveredNode = nullptr;
            return;
        }
    }
    else if (m_LastHoveredNode->type == NodeType::Genre) {
        m_CachedTooltipText = "Genre: " + graphRenderer->getGenreNameByNodeId(m_LastHoveredNode->id) +
            "\nBooks: " + std::to_string(graphRenderer->getNumOfConnectedBooks(m_LastHoveredNode->id)) +
            "\nGenreID: " + std::to_string(m_LastHoveredNode->id);
    }

    // Rozdìlení na øádky a výpoèet velikosti boxu (Layout)
    const int fontSize = 20;
    const int padding = 16;
    const int lineSpacing = 12;

    m_CachedLines.clear();
    size_t start = 0, end;
    while ((end = m_CachedTooltipText.find('\n', start)) != std::string::npos) {
        m_CachedLines.push_back(m_CachedTooltipText.substr(start, end - start));
        start = end + 1;
    }
    m_CachedLines.push_back(m_CachedTooltipText.substr(start));

    int maxLineWidth = 0;
    for (const std::string& line : m_CachedLines) {
        // Použijte MeasureTextEx s vaší font promìnnou a spacingem 2
        int width = MeasureTextEx(m_font, line.c_str(), (float)fontSize, 2).x;
        if (width > maxLineWidth) maxLineWidth = width;
    }

    m_CachedBoxWidth = maxLineWidth + 2 * padding;
    m_CachedBoxHeight = (fontSize + lineSpacing) * (int)m_CachedLines.size() - lineSpacing + 2 * padding;
}


void UIManager::Draw(Vector2 mousePos,  GraphManager* graphRenderer, const BookManager& bookManager) const
{
    // Vykreslení UI prvkù
    DrawButtons(mousePos);
    DrawHelpText();

    // Vykreslení FPS
    DrawFPS(10, m_const.screenHeight - 20);

    // Vykreslení poètu uzlù (vyžaduje GraphManager)
    if (graphRenderer != nullptr) {
        std::string nodeAmountText = "Node amount: " + std::to_string(graphRenderer->getNodes().size());
        DrawText(nodeAmountText.c_str(), 10, m_const.screenHeight - 40, 20, BLACK);
    }

    // Vykreslení tooltipu (vyžaduje logiku výpoètu v Draw)
    DrawTooltip(mousePos);
}

// --- PRIVÁTNÍ METODA: Vykreslení tlaèítek ---
void UIManager::DrawButtons(Vector2 mousePos) const
{
    // ULOŽIT
    bool saveHover = CheckCollisionPointRec(mousePos, m_const.saveButton);
    DrawRectangleRec(m_const.saveButton, saveHover ? DARKGRAY : LIGHTGRAY);
    DrawTextEx(m_font,"Save", { m_const.saveButton.x + m_const.saveButton.width / 2 - MeasureTextEx(m_font, "Save", 20, 2).x / 2, m_const.saveButton.y + 10 }, 20, 2, BLACK);

    // NAÈÍST
    bool loadHover = CheckCollisionPointRec(mousePos, m_const.loadButton);
    DrawRectangleRec(m_const.loadButton, loadHover ? DARKGRAY : LIGHTGRAY);
    DrawTextEx(m_font,"Load", { m_const.loadButton.x + m_const.loadButton.width / 2 - MeasureTextEx(m_font, "Load", 20, 2).x / 2, m_const.loadButton.y + 10 }, 20, 2, BLACK);
}


void UIManager::DrawButton(const std::string& text) const
{

}

// --- PRIVÁTNÍ METODA: Vykreslení nápovìdy ---
void UIManager::DrawHelpText() const
{
    DrawText("Right-click and drag to move nodes", 10, 10, 20, BLACK);
    DrawText("Press SPACE to add 50 new books", 10, 40, 20, BLACK);
    DrawText("Hold SHIFT while dragging to lock a node", 10, 70, 20, BLACK);
    DrawText("Middle click to pan around", 10, 100, 20, BLACK);
    DrawText("Double click a node to unlock it", 10, 130, 20, BLACK);
    DrawText("Press V to unlock framerate", 10, 160, 20, BLACK);
    DrawText("Press B to enable VSYNC", 10, 190, 20, BLACK);
}

// --- PRIVÁTNÍ METODA: Vykreslení tooltipu ---
void UIManager::DrawTooltip(Vector2 mousePos) const
{
    if (m_LastHoveredNode && (GetTime() - m_HoverStartTime >= 0.5) && !IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        // Vykreslení boxu
        DrawRectangleRounded({ mousePos.x + 10, mousePos.y + 10, (float)m_CachedBoxWidth + 20, (float)m_CachedBoxHeight + 20 }, 0.2f, 10, Fade(NookCol::POPUP_BORDER, 0.75f));
        DrawRectangleRounded({ mousePos.x + 10, mousePos.y + 10, (float)m_CachedBoxWidth + 16, (float)m_CachedBoxHeight + 16 }, 0.2f, 10, Fade(NookCol::POPUP_BG, 0.95f));

        const int fontSize = 20;
        const int padding = 16;
        const int lineSpacing = 12;
        int yOffset = 0;

        // Vykreslení textu
        for (const std::string& line : m_CachedLines) {
            size_t colonPos = line.find(':');

            // Posun o 10, protože box je posunut o 10
            float xStart = mousePos.x + 10 + padding;
            float yStart = mousePos.y + 10 + padding + yOffset;

            if (colonPos != std::string::npos) {
                std::string label = line.substr(0, colonPos + 1);
                std::string value = line.substr(colonPos + 1);

                DrawTextEx(m_font, label.c_str(), { xStart, yStart }, (float)fontSize, 2, NookCol::TEXT_HIGHLIGHT);
                int labelWidth = MeasureTextEx(m_font, label.c_str(), (float)fontSize, 2).x;
                DrawTextEx(m_font, value.c_str(), { xStart + labelWidth, yStart }, (float)fontSize, 2, NookCol::TEXT_DEFAULT);
            }
            else {
                DrawTextEx(m_font, line.c_str(), { xStart, yStart }, (float)fontSize, 2, NookCol::TEXT_DEFAULT);
            }
            yOffset += fontSize + lineSpacing;
        }
    }
}


