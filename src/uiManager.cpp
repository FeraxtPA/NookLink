#include "uiManager.h"
#include "UI/button.h"
#include "UI/textInput.h" 
#include "UI/panel.h"    
#include "colors.h"
#include <iostream>

UIManager::UIManager(int screenWidth, int screenHeight)
    : m_ScreenWidth(screenWidth), m_ScreenHeight(screenHeight)
{}

UIManager::~UIManager() {}

void UIManager::Update(Vector2 worldMousePos, Vector2 mousePos, BookManager& bookManager, GraphManager* graphRenderer, TextRenderer* textRenderer)
{
    
    m_LastMousePos = mousePos;

    if (graphRenderer != nullptr) {
        Node* newlyHovered = graphRenderer->getNodeAtPosition(worldMousePos);
        if (newlyHovered != m_LastHoveredNode) {
            m_LastHoveredNode = newlyHovered;
            m_HoverStartTime = GetTime(); 
            m_CachedTooltipText.clear(); 
        }
    }

    // 2. Update Widgets
    for (auto it = m_Widgets.rbegin(); it != m_Widgets.rend(); ++it) {
        (*it)->Update();
    }
    // 3. Update Tooltip (Measure text if needed)
    UpdateTooltipCache(graphRenderer, bookManager, textRenderer);
}

void UIManager::UpdateTooltipCache(const GraphManager* graphRenderer, const BookManager& bookManager, TextRenderer* textRenderer)
{
    if (m_LastHoveredNode == nullptr || m_CachedTooltipText.empty() == false || textRenderer == nullptr) return;

    
    if (m_LastHoveredNode->type == NodeType::Book) {
        const Book* b = bookManager.findBookById(m_LastHoveredNode->id);
        if (b) {
            m_CachedTooltipText = "Title: " + b->getTitle() +
                "\nAuthor: " + b->getAuthor() +
                "\nStatus: " + statusToString(b->getStatus()) +
                "\nRating: " + Book::ratingToStars(b->getRating());
        }
    }
    else if (m_LastHoveredNode->type == NodeType::Genre) {
        m_CachedTooltipText = "Genre: " + graphRenderer->getGenreNameByNodeId(m_LastHoveredNode->id) +
            "\nBooks: " + std::to_string(graphRenderer->getNumOfConnectedBooks(m_LastHoveredNode->id));
    }

    // Calculate Layout
    const float fontSize = 20.0f;
    const int padding = 16;
    const int lineSpacing = 12;

    m_CachedLines.clear();
    size_t start = 0, end;
    while ((end = m_CachedTooltipText.find('\n', start)) != std::string::npos) {
        m_CachedLines.push_back(m_CachedTooltipText.substr(start, end - start));
        start = end + 1;
    }
    m_CachedLines.push_back(m_CachedTooltipText.substr(start));

    float maxLineWidth = 0;
    for (const std::string& line : m_CachedLines) {
        // Use the renderer to measure properly!
        float width = textRenderer->Measure(line, fontSize);
        if (width > maxLineWidth) maxLineWidth = width;
    }

    m_CachedBoxWidth = (int)maxLineWidth + 2 * padding;
    m_CachedBoxHeight = (int)((fontSize + lineSpacing) * m_CachedLines.size()) - lineSpacing + 2 * padding;
}

void UIManager::Draw(Vector2 mousePos, GraphManager* graphRenderer, const BookManager& bookManager, TextRenderer* textRenderer) const
{
    if (!textRenderer) return;

    DrawHelpText(textRenderer);

    // Draw FPS
    textRenderer->DrawSimpleText(std::to_string(GetFPS()), { 10, (float)m_ScreenHeight - 20 }, 20, GREEN);

    // Draw Widgets
    for (auto& w : m_Widgets) w->Draw(textRenderer);

    // Draw Node Count
    if (graphRenderer != nullptr) {
        std::string count = "Nodes: " + std::to_string(graphRenderer->getNodes().size());
        textRenderer->DrawSimpleText(count, { 10, (float)m_ScreenHeight - 40 }, 20, BLACK);
    }

    if (!IsMouseOverUI())
    {
        DrawTooltip(mousePos, textRenderer);
    }
    
}

void UIManager::DrawHelpText(TextRenderer* renderer) const
{
   
    renderer->DrawSimpleText("Right-click drag: Move | Space: Add Books", { 10, 10 }, 20, BLACK);

  
    renderer->DrawSimpleText("Shift+Drag: Lock | Shift+Click: Delete", { 10, 35 }, 20, BLACK);

   
    renderer->DrawSimpleText("Middle Click: Pan | Scroll: Zoom", { 10, 60 }, 20, BLACK);

   
    renderer->DrawSimpleText("Double Click: Unlock Node", { 10, 85 }, 20, BLACK);

   
    renderer->DrawSimpleText("V: Unlock FPS | B: Enable VSync", { 10, 110 }, 20, BLACK);
}

void UIManager::DrawTooltip(Vector2 mousePos, TextRenderer* renderer) const
{
    if (m_LastHoveredNode && (GetTime() - m_HoverStartTime >= 0.5) && !IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {

        DrawRectangleRounded({ mousePos.x + 10, mousePos.y + 10, (float)m_CachedBoxWidth + 20, (float)m_CachedBoxHeight + 20 }, 0.2f, 10, Fade(NookCol::POPUP_BORDER, 0.75f));
        DrawRectangleRounded({ mousePos.x + 10, mousePos.y + 10, (float)m_CachedBoxWidth + 16, (float)m_CachedBoxHeight + 16 }, 0.2f, 10, Fade(NookCol::POPUP_BG, 0.95f));

        float xStart = mousePos.x + 26;
        float yStart = mousePos.y + 26;
        float spacing = 32;

        for (const auto& line : m_CachedLines) {
            renderer->DrawSimpleText(line, { xStart, yStart }, 20, NookCol::TEXT_DEFAULT);
            yStart += spacing;
        }
    }
}

bool UIManager::IsMouseOverUI() const {
    for (const auto& w : m_Widgets) {
        if (w->isVisible && w->isHovered) return true;
    }
    return false;
}
void UIManager::BuildInterface(
    std::function<void()> onSave,
    std::function<void()> onLoad,
    std::function<void(std::string title, std::string author, std::string genres, float rating, Status status, std::string notes)> onAddBook)
{
    // 1. Save/Load Buttons
    m_Widgets.push_back(std::make_shared<Button>(
        Rectangle{ (float)m_ScreenWidth - 220, 10, 100, 40 }, "Save", onSave
    ));
    m_Widgets.push_back(std::make_shared<Button>(
        Rectangle{ (float)m_ScreenWidth - 110, 10, 100, 40 }, "Load", onLoad
    ));

    // 2. "Add Book" Panel
    float cx = m_ScreenWidth / 2.0f;
    float cy = m_ScreenHeight / 2.0f;
    float panelW = 400;
    float panelH = 550;

    auto panel = std::make_shared<Panel>(
        Rectangle{ cx - panelW / 2, cy - panelH / 2, panelW, panelH }, "Add New Book"
    );
    panel->isVisible = false;

    // Layout helper variables
    float startY = -220; // Start higher up
    float gap = 55;
    float inputH = 35;
    float inputW = 360;
    float xOff = -180;

    // --- INPUT FIELDS ---
    auto inTitle = std::make_shared<TextInput>(Rectangle{ cx + xOff, cy + startY, inputW, inputH }, "Title");
    auto inAuthor = std::make_shared<TextInput>(Rectangle{ cx + xOff, cy + startY + gap, inputW, inputH }, "Author");
    auto inGenres = std::make_shared<TextInput>(Rectangle{ cx + xOff, cy + startY + gap * 2, inputW, inputH }, "Genres (e.g. SciFi, Horror)");
    auto inRating = std::make_shared<TextInput>(Rectangle{ cx + xOff, cy + startY + gap * 3, inputW, inputH }, "Rating (0.0 - 5.0)");
    auto inNotes = std::make_shared<TextInput>(Rectangle{ cx + xOff, cy + startY + gap * 4, inputW, inputH }, "Notes...");

    // --- STATUS TOGGLE BUTTON ---
    // We use a shared int to track state: 0=ToRead, 1=Reading, 2=Read
    auto statusState = std::make_shared<int>(0);

    // We create the pointer first so we can capture it in the lambda
    auto btnStatus = std::make_shared<Button>(
        Rectangle{ cx + xOff, cy + startY + gap * 5, inputW, 40 },
        "Status: To Read",
        []() {}
    );
    std::weak_ptr<Button> weakBtn = btnStatus;

    btnStatus->SetOnClick([statusState, weakBtn]() {
        // Try to lock the pointer (check if button still exists)
        if (auto btn = weakBtn.lock()) {
            // Cycle state
            *statusState = (*statusState + 1) % 3;

            // Update Text
            if (*statusState == 0) btn->SetText("Status: To Read");
            else if (*statusState == 1) btn->SetText("Status: Reading");
            else btn->SetText("Status: Read");
        }
        });
    // --- CREATE BUTTON ---
    auto btnCreate = std::make_shared<Button>(
        Rectangle{ cx + xOff, cy + startY + gap * 7, 100, 40 }, "Create",
        [panel, inTitle, inAuthor, inGenres, inRating, inNotes, statusState, onAddBook]() {
            std::string t = inTitle->GetText();
            std::string a = inAuthor->GetText();
            std::string g = inGenres->GetText();
            std::string rStr = inRating->GetText();
            std::string n = inNotes->GetText();

            // Minimal validation
            if (!t.empty() && !a.empty()) {
                // Parse Rating safely
                float r = 0.0f;
                try { r = std::stof(rStr); }
                catch (...) { r = 0.0f; }

                // Map state int to Enum
                Status s = Status::ToRead;
                if (*statusState == 1) s = Status::Reading;
                if (*statusState == 2) s = Status::Read;

                // Call Application Logic
                onAddBook(t, a, g, r, s, n);

                // Clear and Hide
                inTitle->Clear(); inAuthor->Clear(); inGenres->Clear();
                inRating->Clear(); inNotes->Clear();
                panel->isVisible = false;
            }
        }
    );

    // --- CANCEL BUTTON ---
    auto btnCancel = std::make_shared<Button>(
        Rectangle{ cx + 80, cy + startY + gap * 7, 100, 40 }, "Cancel",
        // Capture all input fields so we can clear them
        [panel, inTitle, inAuthor, inGenres, inRating, inNotes]() {
            // 1. Clear all fields
            inTitle->Clear();
            inAuthor->Clear();
            inGenres->Clear();
            inRating->Clear();
            inNotes->Clear();

            // 2. Hide the panel
            panel->isVisible = false;
        }
    );

    // Add children to panel
    panel->AddChild(inTitle);
    panel->AddChild(inAuthor);
    panel->AddChild(inGenres);
    panel->AddChild(inRating);
    panel->AddChild(inNotes);
    panel->AddChild(btnStatus);
    panel->AddChild(btnCreate);
    panel->AddChild(btnCancel);

    // 3. Open Button
    m_Widgets.push_back(std::make_shared<Button>(
        Rectangle{ (float)m_ScreenWidth - 380, 10, 150, 40 }, "Add Book",
        [panel]() { panel->isVisible = true; }
    ));

    // Add panel last so it draws on top
    m_Widgets.push_back(panel);
}