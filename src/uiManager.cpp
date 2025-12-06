#include "uiManager.h"
#include "UI/button.h"
#include "UI/textInput.h" 
#include "UI/panel.h"    
#include "UI/textBox.h"
#include "colors.h"
#include <iostream>
#include <numeric>
#include <format> 
#include "UI/widget.h"

UIManager::UIManager(int screenWidth, int screenHeight)
    : m_ScreenWidth(screenWidth), m_ScreenHeight(screenHeight)
{}

UIManager::~UIManager() {}

void UIManager::OpenEditPanel(Book* book)
{
    if (!book || !m_EditPanel) return;

    m_EditingBookId = book->getId();

    m_EditTitle->text = book->getTitle();
    m_EditAuthor->text = book->getAuthor();

    
    m_EditRating->text = std::format("{:.2f}", book->getRating());

    
    m_EditNotes->SetText(book->getNotes());

    // Genres
    std::string genreStr = "";
    const auto& genres = book->getGenres();
    for (size_t i = 0; i < genres.size(); ++i) {
        genreStr += genres[i];
        if (i < genres.size() - 1) genreStr += ", ";
    }
    m_EditGenres->text = genreStr;

    // Status
    *m_EditStatusState = (int)book->getStatus();
    if (*m_EditStatusState == 0) m_EditStatusBtn->SetText("Status: To Read");
    else if (*m_EditStatusState == 1) m_EditStatusBtn->SetText("Status: Reading");
    else m_EditStatusBtn->SetText("Status: Read");

    m_EditPanel->isVisible = true;
}


MouseCursor Widget::DesiredCursor = MOUSE_CURSOR_DEFAULT;

void UIManager::Update(Vector2 worldMousePos, Vector2 mousePos, BookManager& bookManager, GraphManager* graphRenderer, TextRenderer* textRenderer)
{
    m_LastMousePos = mousePos;

    Widget::DesiredCursor = MOUSE_CURSOR_DEFAULT;

    if (graphRenderer != nullptr) {
        Node* newlyHovered = graphRenderer->getNodeAtPosition(worldMousePos);
        if (newlyHovered != m_LastHoveredNode) {
            m_LastHoveredNode = newlyHovered;
            m_HoverStartTime = GetTime();
            m_CachedTooltipText.clear();
        }
    }

    // Update Widgets
    // Reverse order so top-most panels capture input first
    for (auto it = m_Widgets.rbegin(); it != m_Widgets.rend(); ++it) {
        (*it)->Update();
    }
    if (!IsMouseOverUI())
    {
        
        UpdateTooltipCache(graphRenderer, bookManager, textRenderer);
    }

    if (!IsMouseOverUI())
    {
        SetMouseCursor(Widget::DesiredCursor);
    }

    

    
    
}

void UIManager::UpdateTooltipCache(const GraphManager* graphRenderer, const BookManager& bookManager, TextRenderer* textRenderer)
{
    if (m_LastHoveredNode == nullptr || m_CachedTooltipText.empty() == false || textRenderer == nullptr) return;

    if (m_LastHoveredNode->type == NodeType::Book) {
        const Book* b = bookManager.findBookById(m_LastHoveredNode->id);
        if (b) {
            // Using accumulate to join strings
            std::string genres = "";
            for (size_t i = 0; i < b->getGenres().size(); ++i) {
                genres += b->getGenres()[i] + (i < b->getGenres().size() - 1 ? ", " : "");
            }

            m_CachedTooltipText = "Title: " + b->getTitle() +
                "\nAuthor: " + b->getAuthor() +
                "\nStatus: " + statusToString(b->getStatus()) +
                "\nRating: " + Book::ratingToStars(b->getRating()) +
                "\nGenres: " + genres +
                "\nNotes: " + b->getNotes() +
                "\n(ID: " + std::to_string(b->getId()) + ")";
        }
    }
    else if (m_LastHoveredNode->type == NodeType::Genre) {
        m_CachedTooltipText = "Genre: " + graphRenderer->getGenreNameByNodeId(m_LastHoveredNode->id) +
            "\nBooks: " + std::to_string(graphRenderer->getNumOfConnectedBooks(m_LastHoveredNode->id)) +
            "\n(ID: " + std::to_string(m_LastHoveredNode->id) + ")";
    }

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
    textRenderer->DrawSimpleText(std::to_string(GetFPS()), { 10, (float)m_ScreenHeight - 20 }, 20, GREEN);

    for (auto& w : m_Widgets) w->Draw(textRenderer);

    if (graphRenderer != nullptr) {
        std::string count = "Nodes: " + std::to_string(graphRenderer->getNodes().size());
        textRenderer->DrawSimpleText(count, { 10, (float)m_ScreenHeight - 40 }, 20, BLACK);
    }

    if (!IsMouseOverUI()) {
        DrawTooltip(mousePos, textRenderer);
    }
}

void UIManager::DrawHelpText(TextRenderer* renderer) const
{
    renderer->DrawSimpleText("Right-click drag: Move | Space: Add Books", { 10, 10 }, 20, BLACK);
    renderer->DrawSimpleText("Shift+Drag: Lock | Shift+Click: Delete", { 10, 35 }, 20, BLACK);
    renderer->DrawSimpleText("Middle Click: Pan | Scroll: Zoom", { 10, 60 }, 20, BLACK);
    renderer->DrawSimpleText("Double Click: Unlock Node | 'E': Edit Node", { 10, 85 }, 20, BLACK);
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
    std::function<void(std::string, std::string, std::string, float, Status, std::string)> onAddBook,
    std::function<void(int, std::string, std::string, std::string, float, Status, std::string)> onEditBook)
{
    
    m_Widgets.push_back(std::make_shared<Button>(
        Rectangle{ (float)m_ScreenWidth - 220, 10, 100, 40 }, "Save", onSave
    ));
    m_Widgets.push_back(std::make_shared<Button>(
        Rectangle{ (float)m_ScreenWidth - 110, 10, 100, 40 }, "Load", onLoad
    ));

    // Layout
    float cx = m_ScreenWidth / 2.0f;
    float cy = m_ScreenHeight / 2.0f;
    float panelW = 400;
    float panelH = 550;
    float startY = -220;
    float gap = 55;
    float inputH = 35;
    float inputW = 360;
    float xOff = -180;

    //Book Add Panel
    auto addPanel = std::make_shared<Panel>(
        Rectangle{ cx - panelW / 2, cy - panelH / 2, panelW, panelH }, "Add New Book"
    );
    addPanel->isVisible = false;

    auto inTitle = std::make_shared<TextInput>(Rectangle{ cx + xOff, cy + startY, inputW, inputH }, "Title");
    auto inAuthor = std::make_shared<TextInput>(Rectangle{ cx + xOff, cy + startY + gap, inputW, inputH }, "Author");
    auto inGenres = std::make_shared<TextInput>(Rectangle{ cx + xOff, cy + startY + gap * 2, inputW, inputH }, "Genres (e.g. SciFi, Horror)");
    auto inRating = std::make_shared<TextInput>(Rectangle{ cx + xOff, cy + startY + gap * 3, inputW, inputH }, "Rating (0.0 - 5.0)");

    //Text Box for notes
    auto inNotes = std::make_shared<TextBox>(
        Rectangle{ cx + xOff, cy + startY + gap * 4, inputW, 100 },
        "Notes"
    );

    // Add Status Button
    auto addStatusState = std::make_shared<int>(0);
    auto btnAddStatus = std::make_shared<Button>(
        Rectangle{ cx + xOff, cy + startY + gap * 6 + 20, inputW, 40 }, "Status: To Read", []() {} 
    );

    std::weak_ptr<Button> weakAddBtn = btnAddStatus;
    btnAddStatus->SetOnClick([addStatusState, weakAddBtn]() {
        if (auto btn = weakAddBtn.lock()) {
            *addStatusState = (*addStatusState + 1) % 3;
            if (*addStatusState == 0) btn->SetText("Status: To Read");
            else if (*addStatusState == 1) btn->SetText("Status: Reading");
            else btn->SetText("Status: Read");
        }
        });

    auto btnCreate = std::make_shared<Button>(
        Rectangle{ cx + xOff, cy + startY + gap * 8, 100, 40 }, "Create",
        [addPanel, inTitle, inAuthor, inGenres, inRating, inNotes, addStatusState, onAddBook]() {
            std::string t = inTitle->GetText();
            std::string a = inAuthor->GetText();
            std::string g = inGenres->GetText();
            std::string rStr = inRating->GetText();
            std::string n = inNotes->GetText();

            if (!t.empty() && !a.empty()) {
                float r = 0.0f;
                try { r = std::stof(rStr); }
                catch (...) { r = 0.0f; }

                Status s = Status::ToRead;
                if (*addStatusState == 1) s = Status::Reading;
                if (*addStatusState == 2) s = Status::Read;

                onAddBook(t, a, g, r, s, n);

                
                inTitle->Clear(); inAuthor->Clear(); inGenres->Clear();
                inRating->Clear(); inNotes->Clear();
                addPanel->isVisible = false;
            }
        }
    );

    auto btnCancelAdd = std::make_shared<Button>(
        Rectangle{ cx + 80, cy + startY + gap * 8, 100, 40 }, "Cancel",
        [addPanel, inTitle, inAuthor, inGenres, inRating, inNotes]() {
            inTitle->Clear(); inAuthor->Clear(); inGenres->Clear();
            inRating->Clear(); inNotes->Clear();
            addPanel->isVisible = false;
        }
    );

    addPanel->AddChild(inTitle);
    addPanel->AddChild(inAuthor);
    addPanel->AddChild(inGenres);
    addPanel->AddChild(inRating);
    addPanel->AddChild(inNotes);
    addPanel->AddChild(btnAddStatus);
    addPanel->AddChild(btnCreate);
    addPanel->AddChild(btnCancelAdd);

    // Edit book panel
    m_EditPanel = std::make_shared<Panel>(
        Rectangle{ cx - panelW / 2, cy - panelH / 2, panelW, panelH }, "Edit Book Details"
    );
    m_EditPanel->isVisible = false;

    m_EditTitle = std::make_shared<TextInput>(Rectangle{ cx + xOff, cy + startY, inputW, inputH }, "Title");
    m_EditAuthor = std::make_shared<TextInput>(Rectangle{ cx + xOff, cy + startY + gap, inputW, inputH }, "Author");
    m_EditGenres = std::make_shared<TextInput>(Rectangle{ cx + xOff, cy + startY + gap * 2, inputW, inputH }, "Genres");
    m_EditRating = std::make_shared<TextInput>(Rectangle{ cx + xOff, cy + startY + gap * 3, inputW, inputH }, "Rating");

    //Text box for notes
    m_EditNotes = std::make_shared<TextBox>(
        Rectangle{ cx + xOff, cy + startY + gap * 4, inputW, 100 },
        "Notes"
    );

    m_EditStatusState = std::make_shared<int>(0);
    m_EditStatusBtn = std::make_shared<Button>(
        Rectangle{ cx + xOff, cy + startY + gap * 6 + 20, inputW, 40 }, "Status", []() {}
    );

    std::weak_ptr<Button> weakEditBtn = m_EditStatusBtn;
    auto editState = m_EditStatusState;

    m_EditStatusBtn->SetOnClick([editState, weakEditBtn]() {
        if (auto btn = weakEditBtn.lock()) {
            *editState = (*editState + 1) % 3;
            if (*editState == 0) btn->SetText("Status: To Read");
            else if (*editState == 1) btn->SetText("Status: Reading");
            else btn->SetText("Status: Read");
        }
        });

    auto btnUpdate = std::make_shared<Button>(
        Rectangle{ cx + xOff, cy + startY + gap * 8, 100, 40 }, "Update",
        [this, onEditBook, editState]() {
            if (m_EditTitle->GetText().empty()) return;

            float r = 0.0f;
            try { r = std::stof(m_EditRating->GetText()); }
            catch (...) {}

            Status s = Status::ToRead;
            if (*editState == 1) s = Status::Reading;
            if (*editState == 2) s = Status::Read;

            onEditBook(
                m_EditingBookId,
                m_EditTitle->GetText(),
                m_EditAuthor->GetText(),
                m_EditGenres->GetText(),
                r, s,
                m_EditNotes->GetText()
            );

            m_EditPanel->isVisible = false;
        }
    );

    auto btnCancelEdit = std::make_shared<Button>(
        Rectangle{ cx + 80, cy + startY + gap * 8, 100, 40 }, "Cancel",
        [this]() { m_EditPanel->isVisible = false; }
    );

    m_EditPanel->AddChild(m_EditTitle);
    m_EditPanel->AddChild(m_EditAuthor);
    m_EditPanel->AddChild(m_EditGenres);
    m_EditPanel->AddChild(m_EditRating);
    m_EditPanel->AddChild(m_EditNotes);
    m_EditPanel->AddChild(m_EditStatusBtn);
    m_EditPanel->AddChild(btnUpdate);
    m_EditPanel->AddChild(btnCancelEdit);

    
    m_Widgets.push_back(std::make_shared<Button>(
        Rectangle{ (float)m_ScreenWidth - 380, 10, 150, 40 }, "Add Book",
        [addPanel]() { addPanel->isVisible = true; }
    ));

    m_Widgets.push_back(addPanel);
    m_Widgets.push_back(m_EditPanel);
}