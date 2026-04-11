
// Main application runtime loop and event handling.
// Coordinates updates, input processing, and rendering loop.


#include "application.h"

#include "colors.h"
#include "logging.h"
#include "constants.h"
#include "../include/tinyfiledialogs/tinyfiledialogs.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <unordered_map>

namespace {
double NowSeconds() {
    return GetTime();
}

std::string ToLowerCopy(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return (char)std::tolower(ch);
    });
    return value;
}

std::string TrimCopy(const std::string& value)
{
    const size_t first = value.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) {
        return "";
    }
    const size_t last = value.find_last_not_of(" \t\n\r");
    return value.substr(first, last - first + 1);
}

bool StartsWith(const std::string& text, const std::string& prefix)
{
    return text.size() >= prefix.size() && text.rfind(prefix, 0) == 0;
}

bool IsAutofocusEligibleSearchQuery(const std::string& normalizedQuery)
{
    bool hasPlainTextSegment = false;

    size_t start = 0;
    while (start <= normalizedQuery.size()) {
        const size_t sep = normalizedQuery.find('|', start);
        const size_t end = (sep == std::string::npos) ? normalizedQuery.size() : sep;

        const std::string segment = TrimCopy(normalizedQuery.substr(start, end - start));
        if (!segment.empty()) {
            const bool isFilterSegment =
                StartsWith(segment, "g:") ||
                StartsWith(segment, "s:") ||
                StartsWith(segment, "fr:") ||
                StartsWith(segment, "finished:") ||
                StartsWith(segment, "r>") ||
                StartsWith(segment, "r<") ||
                StartsWith(segment, "r=") ||
                StartsWith(segment, "r:") ||
                StartsWith(segment, "rating>") ||
                StartsWith(segment, "rating<") ||
                StartsWith(segment, "rating=") ||
                StartsWith(segment, "rating:");

            if (isFilterSegment) {
                return false;
            }

            hasPlainTextSegment = true;
        }

        if (sep == std::string::npos) {
            break;
        }
        start = sep + 1;
    }

    return hasPlainTextSegment;
}
}

void Application::Run()
{
    bool exitApp = false;

    while (!exitApp)
    {
        // Centralized exit gate: keep all save-confirmation behavior in one place.
        if (WindowShouldClose())
        {
            if (!m_BookManager.getBooks().empty()) {
                if (m_HasUnsavedChanges)
                {
                    const int result = tinyfd_messageBox(
                        "Exit NookLink",
                        "Do you want to save your library before exiting?",
                        "yesnocancel",
                        "question",
                        1
                    );

                    if (result == 1) {
                        Log::Info("Saving session to: " + m_SaveFileName.string());
                        if (!m_BookManager.saveBooksToFile(m_SaveFileName.string())) {
                            m_UIManager->ShowNotification("Save before exit failed. Cancelled exit.");
                            exitApp = false;
                            continue;
                        }
                        exitApp = true;
                    }
                    else if (result == 2) {
                        exitApp = true;
                    }
                    else if (result == 0) {
                        exitApp = false;
                    }
                }
                else
                {
                    exitApp = true;
                }
            }
            else {
                exitApp = true;
            }
        }
        if (!exitApp)
        {
            Update();
            Draw();
        }
    }
}

void Application::Shutdown()
{
    if (m_HasShutdown) {
        return;
    }

    Log::Info("Application shutdown started");

    m_InputHandler.reset();
    m_DebugManager.reset();
    m_UIManager.reset();
    m_CameraHandler.reset();
    m_GraphManager.reset();
    m_TextRenderer.reset();

    if (IsWindowReady()) {
        CloseWindow();
    }

    Log::Info("Application shutdown finished");
    Log::Shutdown();

    m_HasShutdown = true;
}

void Application::UpdateStartScreen()
{
    MouseCursor resolvedCursor = MOUSE_CURSOR_DEFAULT;

    auto updateStartButton = [&resolvedCursor](const std::shared_ptr<Button>& button) {
        if (!button || !button->IsVisible()) {
            return;
        }

        button->BeginFrameInput();
        button->Update();

        if (resolvedCursor == MOUSE_CURSOR_DEFAULT) {
            const MouseCursor requested = button->GetRequestedCursor();
            if (requested != MOUSE_CURSOR_DEFAULT) {
                resolvedCursor = requested;
            }
        }
    };

    updateStartButton(m_BtnNewGraph);
    updateStartButton(m_BtnLoadGraph);

    if (m_BtnContinue) {
        const bool canContinue = !m_BookManager.getBooks().empty() || std::filesystem::exists(m_SaveFileName);
        if (canContinue) {
            updateStartButton(m_BtnContinue);
        }
    }

    SetMouseCursor(resolvedCursor);
}

void Application::DrawStartScreen()
{
    const char* title = "NookLink";
    const int fontSize = 64;
    MeasureText(title, fontSize);
    m_TextRenderer->DrawTextCentered(title, { m_ScreenSize.x / 2.0f, m_ScreenSize.y / 4.0f }, fontSize, RAYWHITE);

    if (m_BtnNewGraph) m_BtnNewGraph->Draw(m_TextRenderer.get());
    if (m_BtnLoadGraph) m_BtnLoadGraph->Draw(m_TextRenderer.get());

    const bool canContinue = !m_BookManager.getBooks().empty() || std::filesystem::exists(m_SaveFileName);
    if (m_BtnContinue && canContinue) {
        m_BtnContinue->Draw(m_TextRenderer.get());
    }
}

void Application::Draw()
{
    const double frameCpuStart = NowSeconds();

    BeginDrawing();

    if (m_AppState == AppState::StartScreen)
    {
        ClearBackground(NookCol::BACKGROUND);
        DrawStartScreen();
    }
    else
    {
        ClearBackground(NookCol::BACKGROUND);

        m_CameraHandler->beginMode();

        const Rectangle viewRect = m_GraphManager->getCameraViewRect(m_CameraHandler->getCamera(), m_ScreenSize);

        const double edgesStart = NowSeconds();
        m_GraphManager->drawEdges(m_CameraHandler->getCamera().zoom, viewRect);
        m_ProfileCurrent.drawEdgesMs = (float)((NowSeconds() - edgesStart) * 1000.0);

        const double nodesStart = NowSeconds();
        m_GraphManager->drawNodes(m_CameraHandler->getCamera().zoom, viewRect);
        m_GraphManager->DrawSelectionRectangleWorld();
        m_ProfileCurrent.drawNodesMs = (float)((NowSeconds() - nodesStart) * 1000.0);

        m_CameraHandler->endMode();

        const double uiStart = NowSeconds();
        m_UIManager->Draw(GetMousePosition(), m_GraphManager.get(), m_BookManager, m_TextRenderer.get());
        m_ProfileCurrent.drawUiMs = (float)((NowSeconds() - uiStart) * 1000.0);

        m_ProfileCurrent.frameCpuMs = (float)((NowSeconds() - frameCpuStart) * 1000.0);
        SmoothProfile();

        if (m_ShowProfilingOverlay) {
            DrawProfilingOverlay();
        }
    }

    EndDrawing();
}

std::vector<int> Application::FindSearchFocusBookIds(const std::string& query) const
{
    if (!m_GraphManager || !m_CameraHandler || query.size() < 3) {
        return {};
    }

    std::unordered_map<int, Vector2> nodePositions;
    nodePositions.reserve(m_GraphManager->getNodes().size());
    for (const Node& node : m_GraphManager->getNodes()) {
        if (node.type == NodeType::Book && node.visible) {
            nodePositions[node.id] = node.position;
        }
    }

    const Vector2 camTarget = m_CameraHandler->getCamera().target;

    struct Candidate {
        int bookId = -1;
        int score = 0;
        float distSq = 0.0f;
    };

    std::vector<Candidate> candidates;

    for (const Book& book : m_BookManager.getBooks()) {
        const auto nodeIt = nodePositions.find(book.getId());
        if (nodeIt == nodePositions.end()) {
            continue;
        }

        const std::string title = ToLowerCopy(book.getTitle());
        const std::string author = ToLowerCopy(book.getAuthor());

        int score = 0;
        bool textMatched = false;
        if (title == query) {
            score += 1000;
            textMatched = true;
        }
        else if (StartsWith(title, query)) {
            score += 850;
            textMatched = true;
        }
        else if (title.find(query) != std::string::npos) {
            score += 520;
            textMatched = true;
        }

        if (author == query) {
            score += 420;
            textMatched = true;
        }
        else if (StartsWith(author, query)) {
            score += 260;
            textMatched = true;
        }
        else if (author.find(query) != std::string::npos) {
            score += 130;
            textMatched = true;
        }

        if (!textMatched) {
            continue;
        }

        const int lenDelta = (int)std::abs((int)title.size() - (int)query.size());
        score += std::max(0, 80 - lenDelta * 4);

        if (score <= 0) {
            continue;
        }

        const float dx = nodeIt->second.x - camTarget.x;
        const float dy = nodeIt->second.y - camTarget.y;
        const float distSq = dx * dx + dy * dy;

        candidates.push_back(Candidate{ book.getId(), score, distSq });
    }

    std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        if (a.score != b.score) return a.score > b.score;
        return a.distSq < b.distSq;
    });

    std::vector<int> orderedIds;
    orderedIds.reserve(candidates.size());
    for (const Candidate& c : candidates) {
        orderedIds.push_back(c.bookId);
    }
    return orderedIds;
}

void Application::HandleEditorHotkeys()
{
    if (!m_UIManager || m_AppState != AppState::Editor) {
        return;
    }

    const bool ctrlDown = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    const bool shiftDown = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    const bool suppressTypingHotkeys = m_UIManager->IsSearchBarFocused();

    if (IsKeyPressed(KEY_ESCAPE)) {
        if (m_UIManager->HandleEscapeCloseRequest()) {
            return;
        }
    }

    if (ctrlDown && IsKeyPressed(KEY_F)) {
        m_UIManager->FocusSearchBar();
        m_UIManager->ShowNotification("Search focused (Ctrl+F)", 1.2f);
        return;
    }

    if (ctrlDown && IsKeyPressed(KEY_COMMA)) {
        m_UIManager->ToggleSettingsPanel();
        return;
    }

    if (IsKeyPressed(KEY_F1)) {
        m_UIManager->ToggleSettingsHelpPanel();
        return;
    }

    if (ctrlDown && IsKeyPressed(KEY_N)) {
        m_UIManager->OpenAddPanel();
        return;
    }

    if (ctrlDown && IsKeyPressed(KEY_G)) {
        m_UIManager->ToggleFilterPanel();
        return;
    }

    if (ctrlDown && IsKeyPressed(KEY_S)) {
        if (shiftDown) SaveLibraryAs();
        else SaveLibrary();
        return;
    }

    if (ctrlDown && IsKeyPressed(KEY_O)) {
        LoadLibraryFromCurrentFile();
        return;
    }

    if (ctrlDown && IsKeyPressed(KEY_Z) && !suppressTypingHotkeys) {
        UndoLastAction();
        return;
    }

    if (ctrlDown && IsKeyPressed(KEY_Y) && !suppressTypingHotkeys) {
        RedoLastAction();
        return;
    }
}

void Application::UpdateSearchCameraAssist()
{
    if (!m_UIManager || !m_GraphManager || !m_CameraHandler) {
        return;
    }

    const std::string normalizedQuery = ToLowerCopy(TrimCopy(m_UIManager->GetSearchText()));
    const bool searchJustCleared = normalizedQuery.empty() && !m_LastSearchAutofocusQuery.empty();

    if (normalizedQuery.empty()) {
        if (searchJustCleared) {
            if (m_HasSearchCameraReturnState) {
                m_CameraHandler->SetFocusTarget(m_SearchCameraReturnTarget, m_SearchCameraReturnZoom, false);
                m_HasSearchCameraReturnState = false;
            }
            else {
                m_CameraHandler->ClearFocusTarget();
            }
        }

        m_LastSearchAutofocusQuery.clear();
        m_SearchAutofocusCandidates.clear();
        m_SearchAutofocusCandidateIndex = -1;
        m_UIManager->ClearSearchFocusIndicator();
        return;
    }

    if (!IsAutofocusEligibleSearchQuery(normalizedQuery)) {
        if (normalizedQuery != m_LastSearchAutofocusQuery) {
            m_LastSearchAutofocusQuery = normalizedQuery;
            m_CameraHandler->ClearFocusTarget();
        }
        m_SearchAutofocusCandidates.clear();
        m_SearchAutofocusCandidateIndex = -1;
        m_UIManager->ClearSearchFocusIndicator();
        return;
    }

    const bool searchFocused = m_UIManager->IsSearchBarFocused();
    const bool nextPressed = searchFocused && IsKeyPressed(KEY_RIGHT);
    const bool prevPressed = searchFocused && IsKeyPressed(KEY_LEFT);

    if (normalizedQuery.size() < 3) {
        // For very short queries, result set changes too quickly and causes camera jitter.
        if (normalizedQuery != m_LastSearchAutofocusQuery) {
            m_LastSearchAutofocusQuery = normalizedQuery;
            m_CameraHandler->ClearFocusTarget();
        }
        m_SearchAutofocusCandidates.clear();
        m_SearchAutofocusCandidateIndex = -1;
        m_UIManager->ClearSearchFocusIndicator();
        return;
    }

    const bool queryChanged = normalizedQuery != m_LastSearchAutofocusQuery;

    if (queryChanged) {
        m_SearchAutofocusCandidates = FindSearchFocusBookIds(normalizedQuery);
        m_SearchAutofocusCandidateIndex = m_SearchAutofocusCandidates.empty() ? -1 : 0;
        m_LastSearchAutofocusQuery = normalizedQuery;
    }

    if (m_SearchAutofocusCandidates.empty()) {
        m_UIManager->ClearSearchFocusIndicator();
        return;
    }

    if (m_SearchAutofocusCandidateIndex >= 0) {
        m_UIManager->SetSearchFocusIndicator(
            m_SearchAutofocusCandidateIndex + 1,
            (int)m_SearchAutofocusCandidates.size());
    }

    if (!queryChanged && (nextPressed || prevPressed)) {
        const int count = (int)m_SearchAutofocusCandidates.size();
        if (count > 0) {
            if (m_SearchAutofocusCandidateIndex < 0 || m_SearchAutofocusCandidateIndex >= count) {
                m_SearchAutofocusCandidateIndex = 0;
            }
            if (nextPressed) {
                m_SearchAutofocusCandidateIndex = (m_SearchAutofocusCandidateIndex + 1) % count;
            }
            else if (prevPressed) {
                m_SearchAutofocusCandidateIndex = (m_SearchAutofocusCandidateIndex - 1 + count) % count;
            }
        }
    }

    if (!queryChanged && !nextPressed && !prevPressed) {
        return;
    }

    const int candidateBookId = m_SearchAutofocusCandidates[(size_t)m_SearchAutofocusCandidateIndex];
    Node* node = m_GraphManager->getNodeById(candidateBookId);
    if (!node || node->type != NodeType::Book || !node->visible) {
        m_UIManager->ClearSearchFocusIndicator();
        return;
    }

    const float currentZoom = m_CameraHandler->getCamera().zoom;
    const float desiredZoom = std::clamp(std::max(currentZoom, 1.15f), 0.65f, 1.45f);

    if (!m_HasSearchCameraReturnState) {
        const Camera2D& camera = m_CameraHandler->getCamera();
        m_SearchCameraReturnTarget = camera.target;
        m_SearchCameraReturnZoom = camera.zoom;
        m_HasSearchCameraReturnState = true;
    }

    m_CameraHandler->SetFocusTarget(node->position, desiredZoom, false);
}

void Application::UpdateEditorUi()
{
    m_UIManager->Update(
        m_BookManager,
        m_GraphManager.get());
}

void Application::HandleBulkEdit()
{
    UIManager::BulkEditRequest bulkRequest;
    if (!(m_UIManager->PollBulkEditRequest(bulkRequest) && m_GraphManager)) {
        return;
    }

    const std::vector<int> selectedIds = m_GraphManager->GetMultiSelectedBookIds();
    std::unordered_map<int, Book> beforeById;
    beforeById.reserve(selectedIds.size());
    for (int id : selectedIds) {
        if (Book* b = m_BookManager.getBookById(id)) {
            beforeById[id] = *b;
        }
    }

    int changed = 0;
    auto hasGenreExact = [](const Book& b, const std::string& genre) {
        for (const std::string& g : b.getGenres()) {
            if (g == genre) return true;
        }
        return false;
    };

    for (int id : selectedIds) {
        Book* book = m_BookManager.getBookById(id);
        if (!book) continue;

        bool modified = false;

        if (bulkRequest.removeGenre) {
            if (!bulkRequest.genre.empty() && hasGenreExact(*book, bulkRequest.genre)) {
                std::vector<std::string> remaining;
                remaining.reserve(book->getGenres().size());
                for (const std::string& g : book->getGenres()) {
                    if (g != bulkRequest.genre) remaining.push_back(g);
                }
                book->clearGenres();
                for (const std::string& g : remaining) {
                    book->addGenre(g);
                }
                modified = true;
            }
        }
        else {
            if (!bulkRequest.genre.empty()) {
                const bool allowGenre = !bulkRequest.applyOnlyIfEmpty || book->getGenres().empty();
                if (allowGenre && !hasGenreExact(*book, bulkRequest.genre)) {
                    book->addGenre(bulkRequest.genre);
                    modified = true;
                }
            }

            if (bulkRequest.hasStatus) {
                if (book->getStatus() != bulkRequest.status) {
                    book->setStatus(bulkRequest.status);
                    modified = true;
                }
            }

            if (bulkRequest.hasPublished) {
                const bool allowPublished = !bulkRequest.applyOnlyIfEmpty || TrimCopy(book->getDatePublished()).empty();
                if (allowPublished && book->getDatePublished() != bulkRequest.published) {
                    book->setDatePublished(bulkRequest.published);
                    modified = true;
                }
            }

            if (bulkRequest.hasPages) {
                const bool allowPages = !bulkRequest.applyOnlyIfEmpty || book->getPageCount() <= 0;
                if (allowPages && book->getPageCount() != bulkRequest.pages) {
                    book->setPageCount(bulkRequest.pages);
                    modified = true;
                }
            }
        }

        if (modified) {
            ++changed;
        }
    }

    if (changed > 0) {
        auto sameBulkFields = [](const Book& a, const Book& b) {
            const auto& ga = a.getGenres();
            const auto& gb = b.getGenres();
            if (ga.size() != gb.size()) return false;
            for (size_t i = 0; i < ga.size(); ++i) {
                if (ga[i] != gb[i]) return false;
            }
            return a.getStatus() == b.getStatus()
                && a.getDatePublished() == b.getDatePublished()
                && a.getPageCount() == b.getPageCount();
        };

        std::vector<Book> beforeBatch;
        std::vector<Book> afterBatch;
        for (int id : selectedIds) {
            auto it = beforeById.find(id);
            if (it == beforeById.end()) continue;

            Book* after = m_BookManager.getBookById(id);
            if (!after) continue;

            if (!sameBulkFields(it->second, *after)) {
                beforeBatch.push_back(it->second);
                afterBatch.push_back(*after);
            }
        }

        if (!beforeBatch.empty()) {
            PushHistoryAction(HistoryAction{
                HistoryActionType::BulkEdit,
                -1,
                Book(),
                Book(),
                std::move(beforeBatch),
                std::move(afterBatch)
                });
        }

        m_GraphManager->initializePositions(true);
        const bool shouldWakePhysics = m_GraphManager->HasSignificantNodeOverlaps();
        if (shouldWakePhysics) {
            m_GraphManager->wakeUpPhysics();
        }
        else {
            m_GraphManager->setPhysicsActive(false);
        }
        m_LayoutDirty = shouldWakePhysics;
        m_HasUnsavedChanges = true;
        m_UIManager->MarkAnalyticsDirty();
        m_UIManager->ShowNotification(
            (bulkRequest.removeGenre ? "Bulk remove applied to " : "Bulk edit applied to ") +
            std::to_string(changed) + " books.");
    }
    else {
        m_UIManager->ShowNotification("Bulk edit produced no changes.");
    }
}

void Application::HandleContextMenuActions()
{
    UIManager::NodeContextAction contextAction = UIManager::NodeContextAction::None;
    int contextNodeId = -1;
    if (!(m_UIManager->PollNodeContextAction(contextAction, contextNodeId) && m_GraphManager)) {
        return;
    }

    if (contextAction == UIManager::NodeContextAction::EditBook) {
        Book* selectedBook = m_BookManager.getBookById(contextNodeId);
        if (selectedBook) {
            m_UIManager->OpenEditPanel(selectedBook);
        }
    }
    else if (contextAction == UIManager::NodeContextAction::DeleteBook) {
        const Book* toDelete = m_BookManager.findBookById(contextNodeId);
        if (toDelete) {
            PushHistoryAction(HistoryAction{ HistoryActionType::DeleteBook, contextNodeId, *toDelete, Book() });
            m_BookManager.removeBook(contextNodeId);
            m_GraphManager->initializePositions();
            m_LayoutDirty = true;
            m_HasUnsavedChanges = true;
            m_SettleIterations = 0;
            m_UIManager->ShowNotification("Book deleted.");
            m_UIManager->MarkAnalyticsDirty();
            Log::Info("Deleted book ID via context menu: " + std::to_string(contextNodeId));
        }
    }
    else if (contextAction == UIManager::NodeContextAction::ToggleLock) {
        Node* node = m_GraphManager->getNodeById(contextNodeId);
        if (node) {
            node->locked = !node->locked;
            m_LayoutDirty = true;
            m_UIManager->ShowNotification(node->locked ? "Node locked." : "Node unlocked.");
        }
    }
}

void Application::HandleGraphInput(Vector2 worldMousePos, bool ctrlDown, bool suppressHotkeysForSearch)
{
    UpdateSearchCameraAssist();

    if (!m_UIManager->IsBlockingGraphInteraction())
    {
        bool consumedInput = false;

        if (ctrlDown && IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
            Node* clickedNode = m_GraphManager->getNodeAtPosition(worldMousePos);
            if (clickedNode) {
                m_UIManager->OpenNodeContextMenu(clickedNode->id, clickedNode->type, GetMousePosition(), clickedNode->locked);
                consumedInput = true;
            }
        }

        const bool shiftDeleteAllowed = !(m_InputHandler && m_InputHandler->IsMultiSelectMode());
        if (shiftDeleteAllowed && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && IsKeyDown(KEY_LEFT_SHIFT)) {
            Node* clickedNode = m_GraphManager->getNodeAtPosition(worldMousePos);
            if (clickedNode && clickedNode->type == NodeType::Book) {
                const Book* toDelete = m_BookManager.findBookById(clickedNode->id);
                if (toDelete) {
                    PushHistoryAction(HistoryAction{ HistoryActionType::DeleteBook, clickedNode->id, *toDelete, Book() });
                    m_BookManager.removeBook(clickedNode->id);
                    m_GraphManager->initializePositions();
                    m_LayoutDirty = true;
                    m_HasUnsavedChanges = true;
                    m_SettleIterations = 0;
                    m_UIManager->ShowNotification("Book deleted (Shift+Click). Undo: Ctrl+Z");
                    m_UIManager->MarkAnalyticsDirty();
                    Log::Info("Deleted book ID: " + std::to_string(clickedNode->id));
                    consumedInput = true;
                }
            }
        }

        m_CameraHandler->update();
        if (!consumedInput) {
            m_InputHandler->ProcessInputs(worldMousePos, GetTime(), m_LayoutDirty);
        }
        if (!suppressHotkeysForSearch) {
            m_DebugManager->HandleDebugInputs();
        }
    }

    m_CameraHandler->updateAutoFocus(std::clamp(GetFrameTime(), 0.0f, 0.1f));
}

void Application::Update()
{
    const bool suppressHotkeysForSearch = m_UIManager && m_UIManager->IsSearchBarFocused();

    if (!suppressHotkeysForSearch && IsKeyPressed(KEY_F3)) {
        m_ShowProfilingOverlay = !m_ShowProfilingOverlay;
        Log::Info(std::string("Profiling overlay ") + (m_ShowProfilingOverlay ? "enabled" : "disabled"));
    }

    if (IsWindowResized()) {
        m_ScreenSize.x = (float)GetScreenWidth();
        m_ScreenSize.y = (float)GetScreenHeight();

        m_CameraHandler->updateScreenSize(m_ScreenSize);

        if (m_BtnContinue) m_BtnContinue->OnWindowResize(GetScreenWidth(), GetScreenHeight());
        if (m_BtnNewGraph) m_BtnNewGraph->OnWindowResize(GetScreenWidth(), GetScreenHeight());
        if (m_BtnLoadGraph) m_BtnLoadGraph->OnWindowResize(GetScreenWidth(), GetScreenHeight());

        m_UIManager->OnWindowResize(GetScreenWidth(), GetScreenHeight());
    }

    if (m_AppState == AppState::StartScreen)
    {
        UpdateStartScreen();
    }
    else
    {
        HandleEditorHotkeys();

        const Vector2 worldMousePos = GetScreenToWorld2D(GetMousePosition(), m_CameraHandler->getCamera());
        const bool ctrlDown = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);

        if (IsWindowFocused())
        {
            m_IsUserInteracting =
                IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) ||
                IsMouseButtonReleased(MOUSE_RIGHT_BUTTON) ||
                IsMouseButtonDown(MOUSE_RIGHT_BUTTON) ||
                IsMouseButtonPressed(MOUSE_LEFT_BUTTON) ||
                IsKeyPressed(KEY_SPACE) ||
                IsKeyPressed(KEY_V) ||
                IsKeyPressed(KEY_B);
        }

        if (m_LayoutDirty) {
            if (m_GraphManager) m_GraphManager->wakeUpPhysics();
            m_LayoutDirty = false;
        }

        if (m_GraphManager && m_AppState == AppState::Editor) {
            // Clamp dt to avoid unstable physics after frame stalls.
            const float dt = std::clamp(GetFrameTime(), 0.0f, 0.05f);

            const double physicsStart = NowSeconds();
            m_GraphManager->updatePhysics(dt);
            m_ProfileCurrent.updatePhysicsMs = (float)((NowSeconds() - physicsStart) * 1000.0);
        }

        UpdateEditorUi();
        HandleBulkEdit();
        HandleContextMenuActions();
        HandleGraphInput(worldMousePos, ctrlDown, suppressHotkeysForSearch);
    }
}

void Application::SmoothProfile()
{
    // Exponential smoothing reduces frame-to-frame jitter in the on-screen profiler.
    auto smooth = [this](float current, float& smoothed) {
        smoothed += (current - smoothed) * m_ProfileSmoothing;
    };

    smooth(m_ProfileCurrent.updatePhysicsMs, m_ProfileSmooth.updatePhysicsMs);
    smooth(m_ProfileCurrent.drawEdgesMs, m_ProfileSmooth.drawEdgesMs);
    smooth(m_ProfileCurrent.drawNodesMs, m_ProfileSmooth.drawNodesMs);
    smooth(m_ProfileCurrent.drawUiMs, m_ProfileSmooth.drawUiMs);
    smooth(m_ProfileCurrent.frameCpuMs, m_ProfileSmooth.frameCpuMs);
}

void Application::DrawProfilingOverlay()
{
    if (m_AppState != AppState::Editor) return;

    const int x = NookConst::Render::kProfilingOverlayX;
    const int y = NookConst::Render::kProfilingOverlayY;
    const int w = NookConst::Render::kProfilingOverlayWidth;
    const int h = NookConst::Render::kProfilingOverlayHeight;
    const int fs = NookConst::Render::kProfilingOverlayFontSize;
    const int lineStep = NookConst::Render::kProfilingOverlayLineStep;

    DrawRectangleRounded({ (float)x, (float)y, (float)w, (float)h }, 0.15f, 10, Fade(BLACK, 0.65f));
    DrawRectangleRoundedLinesEx({ (float)x, (float)y, (float)w, (float)h }, 0.15f, 10, 2.0f, Fade(NookCol::UI_BORDER_SOFT, 0.85f));

    const float fps = GetFPS() > 0 ? (float)GetFPS() : 0.0f;
    const float frameBudgetMs = fps > 0.0f ? (1000.0f / fps) : 0.0f;
    const float frameTimeMs = GetFrameTime() * 1000.0f;
    const Camera2D camera = m_CameraHandler ? m_CameraHandler->getCamera() : Camera2D{};
    const bool uiBlockingGraph = m_UIManager ? m_UIManager->IsBlockingGraphInteraction() : false;
    const size_t nodeCount = m_GraphManager ? m_GraphManager->getNodes().size() : 0;
    const size_t edgeCount = m_GraphManager ? m_GraphManager->getEdgeCount() : 0;
    const size_t bookCount = m_BookManager.getBooks().size();

    DrawText(TextFormat("Profiling (F3)"), x + 12, y + 10, fs, RAYWHITE);
    DrawText(TextFormat("physics:   %6.2f ms", m_ProfileSmooth.updatePhysicsMs), x + 12, y + 10 + lineStep * 1, fs, RAYWHITE);
    DrawText(TextFormat("drawEdges: %6.2f ms", m_ProfileSmooth.drawEdgesMs), x + 12, y + 10 + lineStep * 2, fs, RAYWHITE);
    DrawText(TextFormat("drawNodes: %6.2f ms", m_ProfileSmooth.drawNodesMs), x + 12, y + 10 + lineStep * 3, fs, RAYWHITE);
    DrawText(TextFormat("drawUI:    %6.2f ms", m_ProfileSmooth.drawUiMs), x + 12, y + 10 + lineStep * 4, fs, RAYWHITE);
    DrawText(TextFormat("frameCPU:  %6.2f ms", m_ProfileSmooth.frameCpuMs), x + 12, y + 10 + lineStep * 5, fs, NookCol::UI_ACCENT_SOFT);
    DrawText(TextFormat("fps: %3i | budget: %5.2f ms", GetFPS(), frameBudgetMs), x + 12, y + 10 + lineStep * 6, fs, NookCol::UI_TEXT_MUTED);
    DrawText(TextFormat("frameTime: %5.2f ms | delta: %.4f s", frameTimeMs, GetFrameTime()), x + 12, y + 10 + lineStep * 7, fs, NookCol::UI_TEXT_MUTED);
    DrawText(TextFormat("nodes: %zu | edges: %zu | books: %zu", nodeCount, edgeCount, bookCount), x + 12, y + 10 + lineStep * 8, fs, NookCol::UI_TEXT_MUTED);
    DrawText(TextFormat("zoom: %.2fx | cam: (%.1f, %.1f)", camera.zoom, camera.target.x, camera.target.y), x + 12, y + 10 + lineStep * 9, fs, NookCol::UI_TEXT_MUTED);
    DrawText(TextFormat("layoutDirty: %s | settling: %d/%d", m_LayoutDirty ? "yes" : "no", m_SettleIterations, m_MaxSettleIterations), x + 12, y + 10 + lineStep * 10, fs, NookCol::UI_TEXT_MUTED);
    DrawText(TextFormat("uiBlocksGraph: %s | userInput: %s", uiBlockingGraph ? "yes" : "no", m_IsUserInteracting ? "yes" : "no"), x + 12, y + 10 + lineStep * 11, fs, NookCol::UI_TEXT_MUTED);
    DrawText(TextFormat("unsaved: %s | undo: %zu | redo: %zu", m_HasUnsavedChanges ? "yes" : "no", m_UndoHistory.size(), m_RedoHistory.size()), x + 12, y + 10 + lineStep * 12, fs, NookCol::UI_TEXT_MUTED);
}
