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

    float startX = m_FilterPanel->bounds.x + 20;
    float startY = m_FilterPanel->bounds.y + 50;
    float gap = 30;

    float panelX = m_FilterPanel->bounds.x;
    float panelY = m_FilterPanel->bounds.y;
    float panelW = m_FilterPanel->bounds.width;

    std::cout << panelX << ", " << panelY << ", " << panelW << "\n";

    auto closeBtn = std::make_shared<Button>(
        // Position: Top-Right corner of the panel
        Rectangle{ (panelX + panelW/2 - 30), m_FilterPanel->bounds.height, 60, 30},
        "Close",
        [this]() {
            if (m_FilterPanel) m_FilterPanel->isVisible = false;
        }
    );
    m_FilterPanel->AddChild(closeBtn);

    auto addStatusBox = [&](std::string label, Status s) {
        bool isVisible = gm->isStatusVisible(s);

        auto cb = std::make_shared<Checkbox>(
            Rectangle{ startX, startY, 20, 20 },
            label,
            isVisible, // Initial checked state
            [gm, s](bool checked) {
                

                bool currentlyHidden = !gm->isStatusVisible(s);

                // If logic mismatch, toggle it.
                if (checked == currentlyHidden) {
                    gm->toggleStatusVisibility(s);
                }
            }
        );
        m_FilterPanel->AddChild(cb);
        startY += gap;
        };

    addStatusBox("Show 'To Read'", Status::ToRead);
    addStatusBox("Show 'Reading'", Status::Reading);
    addStatusBox("Show 'Read'", Status::Read);

    startY += 10; // Extra spacing

    // genres doesnt work as intended
    /*
    std::vector<std::string> genres = gm->getAllGenreNames();
    std::sort(genres.begin(), genres.end()); // Alphabetical order

    for (const auto& genre : genres) {
        bool isVisible = gm->isGenreVisible(genre);

        auto cb = std::make_shared<Checkbox>(
            Rectangle{ startX, startY, 20, 20 },
            genre,
            isVisible,
            [gm, genre](bool checked) {
                bool currentlyHidden = !gm->isGenreVisible(genre);
                if (checked == currentlyHidden) {
                    gm->toggleGenreVisibility(genre);
                }
            }
        );
        m_FilterPanel->AddChild(cb);
        startY += gap;
    }
    */
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
    
    //Save and load and save as buttons
    m_Widgets.push_back(std::make_shared<Button>(
        Rectangle{ (float)m_ScreenWidth - 220, 10, 100, 40 }, "Save", onSave
    ));
    m_Widgets.push_back(std::make_shared<Button>(
        Rectangle{ (float)m_ScreenWidth - 110, 10, 100, 40 }, "Load", onLoad
    ));

    m_Widgets.push_back(std::make_shared<Button>(
        Rectangle{ (float)m_ScreenWidth - 330, 10, 100, 40 }, "Save As", onSaveAs
    ));

    //Back to menu button
    m_Widgets.push_back(std::make_shared<Button>(
		Rectangle{ static_cast<float>(m_ScreenWidth - 175), static_cast<float>(m_ScreenHeight -50), 150, 40 }, "Back to Menu", onBackToMenu
	));

    // Layout Variables
    float cx = m_ScreenWidth / 2.0f;
    float cy = m_ScreenHeight / 2.0f;
    float panelW = 400;
    float panelH = 550;
    float startY = -220;
    float gap = 55;
    float inputH = 35;
    float inputW = 360;
    float xOff = -180;


    // Search bar
    m_SearchBar = std::make_shared<TextInput>(
        Rectangle{ (float)m_ScreenWidth / 2.0f - 150.0f, 10, 300, 40 },
        "Search Books/Authors..."
    );
    m_Widgets.push_back(m_SearchBar); 



 

  

    m_FilterPanel = std::make_shared<Panel>(
        Rectangle{ 1660, 60, 250, (float)m_ScreenHeight - 80 },
        "Filter Graph"
    );
    m_FilterPanel->isVisible = false;


    auto closeFilterBtn = std::make_shared<Button>(
		Rectangle{ 1660 + 50, 60 + 10, 20, 20 },
		"Close",
		[this]() {
			if (m_FilterPanel) {
				m_FilterPanel->isVisible = false;
			}
		}
	);

    m_FilterPanel->AddChild(closeFilterBtn);


    m_Widgets.push_back(m_FilterPanel);

    


    //Filter button
    m_Widgets.push_back(std::make_shared<Button>(
        Rectangle{ 1130, 10, 120, 40 }, "Filter View",
        [this]() {
            // Toggle visibility of the filter panel
            if (m_FilterPanel) {
                m_FilterPanel->isVisible = !m_FilterPanel->isVisible;

            }
        }
    ));
   

    // ============================================================
    // 2. ADD BOOK PANEL
    // ============================================================
    auto addPanel = std::make_shared<Panel>(
        Rectangle{ cx - panelW / 2, cy - panelH / 2, panelW, panelH }, "Add New Book"
    );
    addPanel->isVisible = false;

    // Inputs
    auto inTitle = std::make_shared<TextInput>(Rectangle{ cx + xOff, cy + startY, inputW, inputH }, "Title");
    auto inAuthor = std::make_shared<TextInput>(Rectangle{ cx + xOff, cy + startY + gap, inputW, inputH }, "Author");
    auto inGenres = std::make_shared<TextInput>(Rectangle{ cx + xOff, cy + startY + gap * 2, inputW, inputH }, "Genres (e.g. SciFi, Horror)");
    auto inRating = std::make_shared<TextInput>(Rectangle{ cx + xOff, cy + startY + gap * 3, inputW, inputH }, "Rating (0.0 - 5.0)");
    auto inNotes = std::make_shared<TextBox>(
        Rectangle{ cx + xOff, cy + startY + gap * 4, inputW, 100 },
        "Notes"
    );

    // Status Button
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

    // Create Button
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

                // Clear & Hide
                inTitle->Clear(); inAuthor->Clear(); inGenres->Clear();
                inRating->Clear(); inNotes->Clear();
                addPanel->isVisible = false;
            }
        }
    );

    // Cancel Button
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

    // ============================================================
    // 3. EDIT BOOK PANEL
    // ============================================================
    m_EditPanel = std::make_shared<Panel>(
        Rectangle{ cx - panelW / 2, cy - panelH / 2, panelW, panelH }, "Edit Book Details"
    );
    m_EditPanel->isVisible = false;

    // Member Inputs
    m_EditTitle = std::make_shared<TextInput>(Rectangle{ cx + xOff, cy + startY, inputW, inputH }, "Title");
    m_EditAuthor = std::make_shared<TextInput>(Rectangle{ cx + xOff, cy + startY + gap, inputW, inputH }, "Author");
    m_EditGenres = std::make_shared<TextInput>(Rectangle{ cx + xOff, cy + startY + gap * 2, inputW, inputH }, "Genres");
    m_EditRating = std::make_shared<TextInput>(Rectangle{ cx + xOff, cy + startY + gap * 3, inputW, inputH }, "Rating");
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

    // ============================================================
    // 4. LOTTERY / RANDOM PICKER PANEL
    // ============================================================
    // Create the Panel
    m_LotteryPanel = std::make_shared<Panel>(
        Rectangle{ cx - 200, cy - 150, 400, 350 }, "Next Read Lottery"
    );
    m_LotteryPanel->isVisible = false;

    // Create Text Display for Animation/Result
    m_LotteryText = std::make_shared<TextBox>(
        Rectangle{ cx - 180, cy - 100, 360, 180 }, "..."
    );
    m_LotteryText->SetEditable(false);

    m_LotteryAutoRead = std::make_shared<Checkbox>(
        Rectangle{ cx - 180, cy + 90, 20, 20 }, // Position below text box
        "Set status to 'Reading' automatically"
    );

    // Close Button (Hidden until winner is picked)
    m_LotteryCloseBtn = std::make_shared<Button>(
        Rectangle{ cx - 50, cy + 130, 100, 40 }, "Close!",
        [this]() { m_LotteryPanel->isVisible = false; }
    );
    m_LotteryCloseBtn->isVisible = false;

    m_LotteryPanel->AddChild(m_LotteryText);
    m_LotteryPanel->AddChild(m_LotteryAutoRead);
    m_LotteryPanel->AddChild(m_LotteryCloseBtn);

 
    // "Pick Random Read" 
    m_Widgets.push_back(std::make_shared<Button>(
        Rectangle{ (float)m_ScreenWidth - 660, 10, 150, 40 }, "Next Read",
        [this]() {
            // Initialize Lottery State
            m_LotteryPanel->isVisible = true;
            m_IsLotteryRolling = true;
            m_LotteryTimer = 2.0f; // Spin for 2 seconds
            m_LotterySpeedTimer = 0.0f;
            m_LotteryCloseBtn->isVisible = false;
            m_LotteryText->SetText("Spinning...");
        }
    ));

    // "Add Book"
    m_Widgets.push_back(std::make_shared<Button>(
        Rectangle{ (float)m_ScreenWidth - 500, 10, 150, 40 }, "Add Book",
        [addPanel]() { addPanel->isVisible = true; }
    ));

    // Panels 
    m_Widgets.push_back(addPanel);
    m_Widgets.push_back(m_EditPanel);
    m_Widgets.push_back(m_LotteryPanel);
}