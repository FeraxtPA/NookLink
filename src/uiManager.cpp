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

    // 1. Graph Interaction
    if (graphRenderer != nullptr) {
        Node* newlyHovered = graphRenderer->getNodeAtPosition(worldMousePos);
        if (newlyHovered != m_LastHoveredNode) {
            m_LastHoveredNode = newlyHovered;
            m_HoverStartTime = GetTime();
            m_CachedTooltipText.clear();
        }
    }

    //Seach bar functionality
    if (graphRenderer) {
        graphRenderer->setSearchQuery(m_SearchBar->GetText());
    }


    if (m_FilterPanel->isVisible && graphRenderer) {
      

        // Ideally, add a boolean `m_FilterPanelDirty` to UIManager.
        // Set it to true when you Add/Edit a book.
        static bool builtOnce = false;
        if (!builtOnce  /*|| dirtyBool*/) {
            RebuildFilterPanel(graphRenderer);
            builtOnce = true;
        }
    }

    
    for (auto it = m_Widgets.rbegin(); it != m_Widgets.rend(); ++it) {
        (*it)->Update();
    }

    // lottery
    if (m_IsLotteryRolling)
    {
        // animation
        float dt = GetFrameTime();
        m_LotteryTimer -= dt;
        m_LotterySpeedTimer -= dt;

        if (m_LotterySpeedTimer <= 0.0f && m_LotteryTimer > 0.0f) {
            m_LotterySpeedTimer = 0.05f;
            const auto& allBooks = bookManager.getBooksToBeRead();
            if (!allBooks.empty()) {
                int r = GetRandomValue(0, (int)allBooks.size() - 1);
                m_LotteryText->SetText("... " + allBooks[r].getTitle() + " ...");
            }
        }

        // picking winner
        if (m_LotteryTimer <= 0.0f) {
            m_IsLotteryRolling = false;

            try {
                const Book& winnerConst = bookManager.getRandomBookToBeRead();
                m_LotteryWinnerId = winnerConst.getId(); 
                m_LastLotteryCheckState = m_LotteryAutoRead->checked; 

                // maybe useless since it doesnt do anything since isrolling is set to false earilier
                if (m_LotteryAutoRead->checked) {
                    Book* winnerMutable = bookManager.getBookById(m_LotteryWinnerId);
                    if (winnerMutable) winnerMutable->setStatus(Status::Reading);
                    if (graphRenderer) graphRenderer->initializePositions();
                }

                //Initial text
                std::string statusMsg = m_LotteryAutoRead->checked ? "\n(Status updated to Reading!)" : "";
                m_LotteryText->SetText(
                    "WINNER!\n\n" +
                    winnerConst.getTitle() + "\n" +
                    "by " + winnerConst.getAuthor() + "\n" +
                    statusMsg
                );
            }
            catch (const std::exception& e) {
                m_LotteryText->SetText("No books found with status 'To Read'!");
                m_LotteryWinnerId = -1;
            }
            m_LotteryCloseBtn->isVisible = true;
        }
    }
    
    // Handling after lotterry is done animating
    else if (m_LotteryPanel->isVisible && m_LotteryWinnerId != -1)
    {
        
        if (m_LotteryAutoRead->checked != m_LastLotteryCheckState)
        {
            m_LastLotteryCheckState = m_LotteryAutoRead->checked; 

            Book* winner = bookManager.getBookById(m_LotteryWinnerId);
            if (winner) {
                
                //update status
                if (m_LotteryAutoRead->checked) {
                    winner->setStatus(Status::Reading);
                }
                else {
                    winner->setStatus(Status::ToRead); // revert if not checked
                }

                
                if (graphRenderer) graphRenderer->initializePositions();

                // Maybe useless aswell since its set earlier needs testing
                std::string statusMsg = m_LotteryAutoRead->checked ? "\n(Status updated to Reading!)" : "";
                m_LotteryText->SetText(
                    "WINNER!\n\n" +
                    winner->getTitle() + "\n" +
                    "by " + winner->getAuthor() + "\n" +
                    statusMsg
                );
            }
        }
    }

    
    if (!IsMouseOverUI()) {
        UpdateTooltipCache(graphRenderer, bookManager, textRenderer);
    }
    if (!IsMouseOverUI()) {
        SetMouseCursor(Widget::DesiredCursor);
    }
}



void UIManager::RebuildFilterPanel(GraphManager* gm)
{
    if (!m_FilterPanel || !gm) return;

    m_FilterPanel->ClearChildren();

    // Kotvíme vše na støed-vpravo (Anchor::CenterRight), stejnì jako samotný FilterPanel.
    // Panel má výšku 800, takže jeho horní okraj je -400 a spodní +400 (od støedu).
    float currentY = -350; // Zaèneme kreslit 50px odshora panelu

    auto closeBtn = std::make_shared<Button>(
        Anchor::CenterRight,
        Vector2{ -125, 360 }, // -125 je støed panelu na X. 360 je dole na ose Y.
        Vector2{ 60, 30 },
        "Close",
        [this]() {
            if (m_FilterPanel) m_FilterPanel->isVisible = false;
        }
    );
    m_FilterPanel->AddChild(closeBtn);

    auto addStatusBox = [&](std::string label, Status s) {
        bool isVisible = gm->isStatusVisible(s);

        auto cb = std::make_shared<Checkbox>(
            Anchor::CenterRight,
            Vector2{ -220, currentY }, // -220 = levý okraj panelu
            Vector2{ 20, 20 },
            label,
            isVisible,
            [gm, s](bool checked) {
                bool currentlyHidden = !gm->isStatusVisible(s);
                if (checked == currentlyHidden) {
                    gm->toggleStatusVisibility(s);
                }
            }
        );
        m_FilterPanel->AddChild(cb);
        currentY += 30; // Posun dolù pro další prvek
        };

    addStatusBox("Show 'To Read'", Status::ToRead);
    addStatusBox("Show 'Reading'", Status::Reading);
    addStatusBox("Show 'Read'", Status::Read);
}

void UIManager::UpdateTooltipCache(const GraphManager* graphRenderer, const BookManager& bookManager, TextRenderer* textRenderer)
{
    if (m_LastHoveredNode == nullptr || m_CachedTooltipText.empty() == false || textRenderer == nullptr) return;

    if (m_LastHoveredNode->type == NodeType::Book) {
        const Book* b = bookManager.findBookById(m_LastHoveredNode->id);
        if (b) {
            
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
    renderer->DrawSimpleText("Right-click drag: Move | Space: Add Books", { 10, 10 }, 20, WHITE);
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
    std::function<void()> onSaveAs,
    std::function<void()> onLoad,
    std::function<void()> onBackToMenu,
    std::function<void(std::string, std::string, std::string, float, Status, std::string)> onAddBook,
    std::function<void(int, std::string, std::string, std::string, float, Status, std::string)> onEditBook)
{
    // ============================================================
    // 1. HLAVNÍ UI (Tlaèítka nahoøe a dole)
    // ============================================================
    // Horní lišta - zprava doleva
    m_Widgets.push_back(std::make_shared<Button>(Anchor::TopRight, Vector2{ -60, 30 }, Vector2{ 100, 40 }, "Load", onLoad));
    m_Widgets.push_back(std::make_shared<Button>(Anchor::TopRight, Vector2{ -170, 30 }, Vector2{ 100, 40 }, "Save", onSave));
    m_Widgets.push_back(std::make_shared<Button>(Anchor::TopRight, Vector2{ -280, 30 }, Vector2{ 100, 40 }, "Save As", onSaveAs));

    // Vyhledávací pole - uprostøed nahoøe
    m_SearchBar = std::make_shared<TextInput>(Anchor::TopCenter, Vector2{ 0, 30 }, Vector2{ 300, 40 }, "Search Books/Authors...");
    m_Widgets.push_back(m_SearchBar);

    // Tlaèítko zpìt do menu - vpravo dole
    m_Widgets.push_back(std::make_shared<Button>(Anchor::BottomRight, Vector2{ -95, -30 }, Vector2{ 150, 40 }, "Back to Menu", onBackToMenu));

    // Filter Panel (vpravo uprostøed) a jeho Toggle Tlaèítko
    m_FilterPanel = std::make_shared<Panel>(Anchor::CenterRight, Vector2{ -125, 0 }, Vector2{ 250, 800 }, "Filter Graph");
    m_FilterPanel->isVisible = false;
    m_Widgets.push_back(m_FilterPanel);

    m_Widgets.push_back(std::make_shared<Button>(
        Anchor::TopRight, Vector2{ -760, 30 }, Vector2{ 120, 40 }, "Filter View",
        [this]() { if (m_FilterPanel) m_FilterPanel->isVisible = !m_FilterPanel->isVisible; }
    ));

    // ============================================================
    // 2. ADD BOOK PANEL (Uprostøed)
    // ============================================================
    auto addPanel = std::make_shared<Panel>(Anchor::Center, Vector2{ 0, 0 }, Vector2{ 400, 550 }, "Add New Book");
    addPanel->isVisible = false;

    auto inTitle = std::make_shared<TextInput>(Anchor::Center, Vector2{ 0, -202.5f }, Vector2{ 360, 35 }, "Title");
    auto inAuthor = std::make_shared<TextInput>(Anchor::Center, Vector2{ 0, -147.5f }, Vector2{ 360, 35 }, "Author");
    auto inGenres = std::make_shared<TextInput>(Anchor::Center, Vector2{ 0, -92.5f }, Vector2{ 360, 35 }, "Genres (e.g. SciFi, Horror)");
    auto inRating = std::make_shared<TextInput>(Anchor::Center, Vector2{ 0, -37.5f }, Vector2{ 360, 35 }, "Rating (0.0 - 5.0)");
    auto inNotes = std::make_shared<TextBox>(Anchor::Center, Vector2{ 0, 50.0f }, Vector2{ 360, 100 }, "Notes");

    auto addStatusState = std::make_shared<int>(0);
    auto btnAddStatus = std::make_shared<Button>(Anchor::Center, Vector2{ 0, 150.0f }, Vector2{ 360, 40 }, "Status: To Read", []() {});

    std::weak_ptr<Button> weakAddBtn = btnAddStatus;
    btnAddStatus->SetOnClick([addStatusState, weakAddBtn]() {
        if (auto btn = weakAddBtn.lock()) {
            *addStatusState = (*addStatusState + 1) % 3;
            if (*addStatusState == 0) btn->SetText("Status: To Read");
            else if (*addStatusState == 1) btn->SetText("Status: Reading");
            else btn->SetText("Status: Read");
        }
        });

    auto btnCreate = std::make_shared<Button>(Anchor::Center, Vector2{ -130, 240 }, Vector2{ 100, 40 }, "Create",
        [addPanel, inTitle, inAuthor, inGenres, inRating, inNotes, addStatusState, onAddBook]() {
            std::string t = inTitle->GetText();
            std::string a = inAuthor->GetText();
            if (!t.empty() && !a.empty()) {
                float r = 0.0f;
                try { r = std::stof(inRating->GetText()); }
                catch (...) { r = 0.0f; }

                Status s = Status::ToRead;
                if (*addStatusState == 1) s = Status::Reading;
                if (*addStatusState == 2) s = Status::Read;

                onAddBook(t, a, inGenres->GetText(), r, s, inNotes->GetText());
                inTitle->Clear(); inAuthor->Clear(); inGenres->Clear(); inRating->Clear(); inNotes->Clear();
                addPanel->isVisible = false;
            }
        }
    );

    auto btnCancelAdd = std::make_shared<Button>(Anchor::Center, Vector2{ 130, 240 }, Vector2{ 100, 40 }, "Cancel",
        [addPanel, inTitle, inAuthor, inGenres, inRating, inNotes]() {
            inTitle->Clear(); inAuthor->Clear(); inGenres->Clear(); inRating->Clear(); inNotes->Clear();
            addPanel->isVisible = false;
        }
    );

    addPanel->AddChild(inTitle); addPanel->AddChild(inAuthor); addPanel->AddChild(inGenres);
    addPanel->AddChild(inRating); addPanel->AddChild(inNotes); addPanel->AddChild(btnAddStatus);
    addPanel->AddChild(btnCreate); addPanel->AddChild(btnCancelAdd);

    // ============================================================
    // 3. EDIT BOOK PANEL (Uprostøed)
    // ============================================================
    m_EditPanel = std::make_shared<Panel>(Anchor::Center, Vector2{ 0, 0 }, Vector2{ 400, 550 }, "Edit Book Details");
    m_EditPanel->isVisible = false;

    m_EditTitle = std::make_shared<TextInput>(Anchor::Center, Vector2{ 0, -202.5f }, Vector2{ 360, 35 }, "Title");
    m_EditAuthor = std::make_shared<TextInput>(Anchor::Center, Vector2{ 0, -147.5f }, Vector2{ 360, 35 }, "Author");
    m_EditGenres = std::make_shared<TextInput>(Anchor::Center, Vector2{ 0, -92.5f }, Vector2{ 360, 35 }, "Genres");
    m_EditRating = std::make_shared<TextInput>(Anchor::Center, Vector2{ 0, -37.5f }, Vector2{ 360, 35 }, "Rating");
    m_EditNotes = std::make_shared<TextBox>(Anchor::Center, Vector2{ 0, 50.0f }, Vector2{ 360, 100 }, "Notes");

    m_EditStatusState = std::make_shared<int>(0);
    m_EditStatusBtn = std::make_shared<Button>(Anchor::Center, Vector2{ 0, 150.0f }, Vector2{ 360, 40 }, "Status", []() {});

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

    auto btnUpdate = std::make_shared<Button>(Anchor::Center, Vector2{ -130, 240 }, Vector2{ 100, 40 }, "Update",
        [this, onEditBook, editState]() {
            if (m_EditTitle->GetText().empty()) return;
            float r = 0.0f; try { r = std::stof(m_EditRating->GetText()); }
            catch (...) {}
            Status s = Status::ToRead;
            if (*editState == 1) s = Status::Reading;
            if (*editState == 2) s = Status::Read;
            onEditBook(m_EditingBookId, m_EditTitle->GetText(), m_EditAuthor->GetText(), m_EditGenres->GetText(), r, s, m_EditNotes->GetText());
            m_EditPanel->isVisible = false;
        }
    );

    auto btnCancelEdit = std::make_shared<Button>(Anchor::Center, Vector2{ 130, 240 }, Vector2{ 100, 40 }, "Cancel",
        [this]() { m_EditPanel->isVisible = false; }
    );

    m_EditPanel->AddChild(m_EditTitle); m_EditPanel->AddChild(m_EditAuthor); m_EditPanel->AddChild(m_EditGenres);
    m_EditPanel->AddChild(m_EditRating); m_EditPanel->AddChild(m_EditNotes); m_EditPanel->AddChild(m_EditStatusBtn);
    m_EditPanel->AddChild(btnUpdate); m_EditPanel->AddChild(btnCancelEdit);

    // ============================================================
    // 4. LOTTERY PANEL (Uprostøed)
    // ============================================================
    m_LotteryPanel = std::make_shared<Panel>(Anchor::Center, Vector2{ 0, 25 }, Vector2{ 400, 350 }, "Next Read Lottery");
    m_LotteryPanel->isVisible = false;

    m_LotteryText = std::make_shared<TextBox>(Anchor::Center, Vector2{ 0, -10 }, Vector2{ 360, 180 }, "...");
    m_LotteryText->SetEditable(false);

    m_LotteryAutoRead = std::make_shared<Checkbox>(Anchor::Center, Vector2{ -170, 100 }, Vector2{ 20, 20 }, "Set status to 'Reading' automatically");

    m_LotteryCloseBtn = std::make_shared<Button>(Anchor::Center, Vector2{ 0, 150 }, Vector2{ 100, 40 }, "Close!",
        [this]() { m_LotteryPanel->isVisible = false; }
    );
    m_LotteryCloseBtn->isVisible = false;

    m_LotteryPanel->AddChild(m_LotteryText); m_LotteryPanel->AddChild(m_LotteryAutoRead); m_LotteryPanel->AddChild(m_LotteryCloseBtn);

    // Pøidání tlaèítek pro spuštìní panelù (horní lišta)
    m_Widgets.push_back(std::make_shared<Button>(Anchor::TopRight, Vector2{ -595, 30 }, Vector2{ 150, 40 }, "Next Read",
        [this]() {
            m_LotteryPanel->isVisible = true; m_IsLotteryRolling = true;
            m_LotteryTimer = 2.0f; m_LotterySpeedTimer = 0.0f;
            m_LotteryCloseBtn->isVisible = false; m_LotteryText->SetText("Spinning...");
        }
    ));

    m_Widgets.push_back(std::make_shared<Button>(Anchor::TopRight, Vector2{ -430, 30 }, Vector2{ 150, 40 }, "Add Book",
        [addPanel]() { addPanel->isVisible = true; }
    ));

    m_Widgets.push_back(addPanel);
    m_Widgets.push_back(m_EditPanel);
    m_Widgets.push_back(m_LotteryPanel);
}