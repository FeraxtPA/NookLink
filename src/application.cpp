

#define _CRT_SECURE_NO_WARNINGS
#include "application.h"
#include "UI/button.h"
#include "logging.h"

#include "../include/tinyfiledialogs/tinyfiledialogs.h"

#include <filesystem>
#include <cstdlib> 

#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <print>
#include <sstream>

namespace fs = std::filesystem;

namespace {
double NowSeconds() {
    return GetTime();
}

std::string GetTodayDateDDMMYYYY()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t tt = std::chrono::system_clock::to_time_t(now);

    std::tm localTm{};
#if defined(_WIN32)
    localtime_s(&localTm, &tt);
#else
    localtime_r(&tt, &localTm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&localTm, "%d.%m.%Y");
    return oss.str();
}

void NormalizeDatesForStatus(Status status, std::string& started, std::string& finished)
{
    const std::string today = GetTodayDateDDMMYYYY();
    if ((status == Status::Reading || status == Status::Read) && started.empty()) {
        started = today;
    }
    if (status == Status::Read && finished.empty()) {
        finished = today;
    }
}
}

void Application::ClearHistory()
{
    m_UndoHistory.clear();
    m_RedoHistory.clear();
}

void Application::PushHistoryAction(const HistoryAction& action)
{
    if (m_UndoHistory.size() >= m_MaxHistoryEntries) {
        m_UndoHistory.erase(m_UndoHistory.begin());
    }
    m_UndoHistory.push_back(action);
    m_RedoHistory.clear();
}

bool Application::ApplyBookSnapshot(const Book& snapshot)
{
    Book* book = m_BookManager.getBookById(snapshot.getId());
    if (!book) {
        Log::Warn("ApplyBookSnapshot failed: missing book ID " + std::to_string(snapshot.getId()));
        return false;
    }

    book->setTitle(snapshot.getTitle());
    book->setAuthor(snapshot.getAuthor());
    book->setStatus(snapshot.getStatus());
    book->setRating(snapshot.getRating());
    book->setNotes(snapshot.getNotes());
    book->setDateAdded(snapshot.getDateAdded());
    book->setDateStartedReading(snapshot.getDateStartedReading());
    book->setDateFinishedReading(snapshot.getDateFinishedReading());
    book->clearGenres();
    for (const auto& genre : snapshot.getGenres()) {
        book->addGenre(genre);
    }

    return true;
}

bool Application::UndoLastAction()
{
    if (m_UndoHistory.empty()) {
        m_UIManager->ShowNotification("Nothing to undo.");
        return false;
    }

    HistoryAction action = m_UndoHistory.back();
    m_UndoHistory.pop_back();

    bool success = false;
    if (action.type == HistoryActionType::AddBook) {
        m_BookManager.removeBook(action.bookId);
        m_GraphManager->initializePositions();
        success = true;
    }
    else if (action.type == HistoryActionType::EditBook) {
        success = ApplyBookSnapshot(action.before);
        if (success) {
            m_GraphManager->updateConnections();
        }
    }
    else if (action.type == HistoryActionType::DeleteBook) {
        success = m_BookManager.restoreBook(action.before);
        if (success) {
            m_GraphManager->initializePositions();
        }
    }

    if (!success) {
        m_UIManager->ShowNotification("Undo failed.");
        Log::Warn("Undo failed");
        return false;
    }

    if (m_RedoHistory.size() >= m_MaxHistoryEntries) {
        m_RedoHistory.erase(m_RedoHistory.begin());
    }
    m_RedoHistory.push_back(action);

    m_LayoutDirty = true;
    m_HasUnsavedChanges = true;
    m_SettleIterations = 0;
    m_UIManager->ShowNotification("Undo applied.");
    Log::Info("Undo applied");
    return true;
}

bool Application::RedoLastAction()
{
    if (m_RedoHistory.empty()) {
        m_UIManager->ShowNotification("Nothing to redo.");
        return false;
    }

    HistoryAction action = m_RedoHistory.back();
    m_RedoHistory.pop_back();

    bool success = false;
    if (action.type == HistoryActionType::AddBook) {
        success = m_BookManager.restoreBook(action.after);
        if (success) {
            m_GraphManager->initializePositions();
        }
    }
    else if (action.type == HistoryActionType::EditBook) {
        success = ApplyBookSnapshot(action.after);
        if (success) {
            m_GraphManager->updateConnections();
        }
    }
    else if (action.type == HistoryActionType::DeleteBook) {
        m_BookManager.removeBook(action.bookId);
        m_GraphManager->initializePositions();
        success = true;
    }

    if (!success) {
        m_UIManager->ShowNotification("Redo failed.");
        Log::Warn("Redo failed");
        return false;
    }

    if (m_UndoHistory.size() >= m_MaxHistoryEntries) {
        m_UndoHistory.erase(m_UndoHistory.begin());
    }
    m_UndoHistory.push_back(action);

    m_LayoutDirty = true;
    m_HasUnsavedChanges = true;
    m_SettleIterations = 0;
    m_UIManager->ShowNotification("Redo applied.");
    Log::Info("Redo applied");
    return true;
}

Application::Application()
{
   
    
    m_UpdateInterval = m_UpdateIntervalInitial;
    LoadConfig();
    
}

Application::~Application()
{
    Shutdown();
}


fs::path getDesktopPath() {
    const char* homeDir = nullptr;

    // We still need preprocessor directives to find the specific ENV variable key
#ifdef _WIN32
    homeDir = std::getenv("USERPROFILE");
#else
    homeDir = std::getenv("HOME");
#endif

    if (homeDir) {
        // C++17: The '/' operator automatically handles platform-specific separators
        // (e.g., adds "\\" on Windows, "/" on Linux/Mac)
        return fs::path(homeDir) / "Desktop";
    }

    // Fallback: return current working directory or empty path
    return fs::current_path();
}

void Application::Initialize()
{
    Log::Init();
    Log::Info("Application initialization started");

    SetConfigFlags(FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(m_ScreenSize.x, m_ScreenSize.y, "NookLink");

    SetExitKey(KEY_NULL);

    Image icon = LoadImage("assets/icon2.png");

    SetWindowIcon(icon);
    UnloadImage(icon);

    m_TextRenderer = std::make_unique<TextRenderer>();


    m_GraphManager = std::make_unique<GraphManager>(m_BookManager, m_ConnectionManager, m_CanvasSize, m_TextRenderer.get());
    m_CameraHandler = std::make_unique<CameraHandler>(m_ScreenSize, m_CanvasSize);

    m_UIManager = std::make_unique<UIManager>(m_ScreenSize.x, m_ScreenSize.y);

    m_InputHandler = std::make_unique<InputHandler>(m_GraphManager.get(), m_BookManager, m_UIManager.get());
    m_DebugManager = std::make_unique<DebugManager>(m_BookManager, m_GraphManager.get());
   
    float centerX = m_ScreenSize.x / 2.0f;
    float centerY = m_ScreenSize.y / 2.0f;


    std::string btnText = "Continue";
    if (fs::exists(m_SaveFileName)) {
        btnText += " (" + m_SaveFileName.filename().string() + ")";
    }

    m_BtnContinue = std::make_shared<Button>(
        Anchor::Center,            // Kotva na st�ed obrazovky
        Vector2{ 0, -140 },        // Odsazen� od st�edu (0 na ose X, 140 pixel� nahoru na ose Y)
        Vector2{ 300, 50 },
        btnText, // Uses the dynamic label
        [this]() {
            // Case 1: In-memory session exists
            if (!m_BookManager.getBooks().empty()) {
                m_AppState = AppState::Editor;
                return;
            }

            // Case 2: Load from the dynamic m_SaveFileName
            if (!fs::exists(m_SaveFileName)) {
                m_UIManager->ShowNotification("Saved library file was not found.");
                Log::Warn("Continue blocked: missing file " + m_SaveFileName.string());
                return;
            }

            Log::Info("Loading library from: " + m_SaveFileName.string());

            std::unordered_map<int, NodePosition> loadedPositions;
            if (!m_BookManager.loadBooksFromFile(m_SaveFileName.string(), loadedPositions)) {
                m_UIManager->ShowNotification("Load failed. Check logs for details.");
                return;
            }

            // 2. Vy�ist�me a inicializujeme graf
            m_GraphManager->clearGenresAndConnections();
            m_GraphManager->initializePositions();

            // 3. APLIKUJEME POZICE (aby uzly nevyst�elily)
            m_GraphManager->applyLoadedPositions(loadedPositions);

            m_LayoutDirty = true;
            ClearHistory();
            m_AppState = AppState::Editor;
        }
    );

    m_BtnNewGraph = std::make_shared<Button>(
        Anchor::Center,            // Kotva na st�ed
        Vector2{ 0, -60 },         // Odsazen� od st�edu (-60 Y)
        Vector2{ 300, 50 },
        "Start New Graph",
        [this]() {
            Log::Info("Starting new graph");
            // Option A: Start completely empty
            m_BookManager = BookManager(); 
            ClearHistory();

            // Option B: Load your demo data (1984, etc.)
            //InitBookManager();

            m_GraphManager->initializePositions();
            m_LayoutDirty = true;
            m_AppState = AppState::Editor; // Switch State
        }
    );


    m_BtnLoadGraph = std::make_shared<Button>(
        Anchor::Center,            // Kotva na st�ed
        Vector2{ 0, 20 },          // Odsazen� od st�edu (+20 Y)
        Vector2{ 300, 50 },
        "Load From File",
        [this]() {
            // 1. Setup paths and filters
            const char* filters[] = { "*.json" };
            fs::path initialPath = getDesktopPath();

            // Ensure the directory actually exists before opening dialog; 
            // otherwise tinyfd might default to a weird location.
            if (!fs::exists(initialPath)) {
                initialPath = fs::current_path();
            }

            // 2. Open the Dialog
            // We use .string().c_str() to convert the C++ path to the const char* required by tinyfd
            const char* selection = tinyfd_openFileDialog(
                "Select a Graph File",
                initialPath.string().c_str(),
                1,
                filters,
                "JSON Files",
                0
            );

            // 3. Check result
            if (selection) {
                // Convert immediately to fs::path for safety and power
                fs::path selectedPath(selection);

                Log::Info("User selected library file: " + selectedPath.string());

                m_SaveFileName = selectedPath; // Assuming m_SaveFileName is fs::path or std::string

                // 4. Perform the Load
                if (fs::exists(m_SaveFileName)) {
                    // 1. Na�teme knihy i pozice
                    std::unordered_map<int, NodePosition> loadedPositions;
                    if (!m_BookManager.loadBooksFromFile(m_SaveFileName.string(), loadedPositions)) {
                        m_UIManager->ShowNotification("Load failed. Check logs for details.");
                        return;
                    }

                    SaveConfig();

                    // 2. Vy�ist�me a inicializujeme graf
                    m_GraphManager->clearGenresAndConnections();
                    m_GraphManager->initializePositions();

                    // 3. APLIKUJEME POZICE
                    m_GraphManager->applyLoadedPositions(loadedPositions);

                    m_LayoutDirty = true;
                    ClearHistory();
                    m_AppState = AppState::Editor;
                }
                else {
                    // This branch is rarely hit if selection is not null, 
                    // but good for sanity checking.
                    Log::Error("Selected file does not exist");
                    m_UIManager->ShowNotification("Selected file does not exist.");
                }
            }
            else {
                Log::Info("Load cancelled by user");
            }
        }
    );

    //Needs to be initialized after font is loaded
   

    m_UIManager->BuildInterface(
        [this]() { // onSave
            if (m_SaveFileName.empty()) {
                m_UIManager->ShowNotification("Save failed: no target file selected.");
                Log::Warn("Save blocked: empty save file name");
                return;
            }

            Log::Info("Saving library to: " + m_SaveFileName.string());
            // Nejd��v vyt�hneme pozice z grafu, pak je po�leme do bookManageru
            auto currentPositions = m_GraphManager->exportPositions();
            if (!m_BookManager.saveBooksToFile(m_SaveFileName.string(), currentPositions)) {
                m_UIManager->ShowNotification("Save failed. Check logs for details.");
                return;
            }
            m_UIManager->ShowNotification("Library Saved!");
            m_HasUnsavedChanges = false;
        },
        [this]() {
            const char* filters[] = { "*.json" };


            fs::path defaultSavePath = getDesktopPath() / "library.json";

            
            // Open the "Save As" Dialog
            // Args: Title, Default File, Num Filters, Filter Patterns, Filter Desc
            const char* path = tinyfd_saveFileDialog(
                "Save Library As...",
                defaultSavePath.string().c_str(),
                1,
                filters,
                "JSON Files"
            );

            if (path) {
                m_SaveFileName = path;
                auto currentPositions = m_GraphManager->exportPositions();
                if (!m_BookManager.saveBooksToFile(m_SaveFileName.string(), currentPositions)) {
                    m_UIManager->ShowNotification("Save As failed. Check logs for details.");
                    return;
                }
                SaveConfig();

                Log::Info("Saved As: " + m_SaveFileName.string());

                m_UIManager->ShowNotification("Saved As: " + m_SaveFileName.filename().string());
                m_HasUnsavedChanges = false;
            }
            else {
                Log::Info("Save As cancelled");
            }
        },
        [this]() { // onLoad
            if (!fs::exists(m_SaveFileName)) {
                m_UIManager->ShowNotification("Load failed: file does not exist.");
                Log::Warn("Load blocked: missing file " + m_SaveFileName.string());
                return;
            }

            Log::Info("Loading books from: " + m_SaveFileName.string());

            // 1. Na�teme knihy i pozice
            std::unordered_map<int, NodePosition> loadedPositions;
            if (!m_BookManager.loadBooksFromFile(m_SaveFileName.string(), loadedPositions)) {
                m_UIManager->ShowNotification("Load failed. Check logs for details.");
                return;
            }

            // 2. Provedeme standardn� �istku grafu
            m_GraphManager->clearGenresAndConnections();
            m_GraphManager->initializePositions();

            // 3. Aplikujeme star� pozice!
            m_GraphManager->applyLoadedPositions(loadedPositions);

            m_LayoutDirty = true;
            SaveConfig();
            ClearHistory();
            m_UIManager->ShowNotification("Library Loaded!");
            m_HasUnsavedChanges = false;
        },
        [this]() { 
            m_AppState = AppState::StartScreen;
            if (m_BtnContinue) {
                std::string btnText = "Continue";
                if (fs::exists(m_SaveFileName)) {
                  
                    btnText += " (" + m_SaveFileName.filename().string() + ")";
                }
                m_BtnContinue->SetText(btnText);
            }
		},
        [this]() { UndoLastAction(); },
        [this]() { RedoLastAction(); },
        [this](std::string title, std::string author, std::string genreStr, float rating, Status status, std::string notes, std::string startedReadingDate, std::string finishedReadingDate) {
            Log::Info("Adding book: " + title);

            NormalizeDatesForStatus(status, startedReadingDate, finishedReadingDate);

           
            Book newBook(title, author, status);
            newBook.setRating(rating);
            newBook.setNotes(notes);
            newBook.setDateStartedReading(startedReadingDate);
            newBook.setDateFinishedReading(finishedReadingDate);

            //Parse Genres (String split by comma)
            std::stringstream ss(genreStr);
            std::string segment;
            while (std::getline(ss, segment, ',')) {
                // Trim leading/trailing spaces
                size_t first = segment.find_first_not_of(' ');
                if (std::string::npos != first) {
                    size_t last = segment.find_last_not_of(' ');
                    std::string cleanGenre = segment.substr(first, (last - first + 1));

                   
                    newBook.addGenre(cleanGenre);
                }
            }

            // Add book to manager and update graph
            const int newId = m_BookManager.addBook(newBook);
            m_GraphManager->initializePositions();

            const Book* addedBook = m_BookManager.findBookById(newId);
            if (addedBook) {
                PushHistoryAction(HistoryAction{ HistoryActionType::AddBook, newId, Book(), *addedBook });
            }

            m_LayoutDirty = true;
            m_SettleIterations = 0;
            m_UIManager->ShowNotification("Book '" + title + "' added!");
            m_HasUnsavedChanges = true;
        },
        //Edit book
        [this](int id, std::string title, std::string author, std::string genreStr, float rating, Status status, std::string notes, std::string startedReadingDate, std::string finishedReadingDate) {
            Log::Info("Editing book ID: " + std::to_string(id));
            NormalizeDatesForStatus(status, startedReadingDate, finishedReadingDate);
            Book* book = m_BookManager.getBookById(id);
            if (book) {
                const Book beforeEdit = *book;
              
                book->setTitle(title);
                book->setAuthor(author);
                book->setRating(rating);
                book->setStatus(status);
                book->setNotes(notes);
                book->setDateStartedReading(startedReadingDate);
                book->setDateFinishedReading(finishedReadingDate);

                book->clearGenres();

                std::stringstream ss(genreStr);
                std::string segment;
                while (std::getline(ss, segment, ',')) {
                    size_t first = segment.find_first_not_of(' ');
                    if (std::string::npos != first) {
                        size_t last = segment.find_last_not_of(' ');
                        book->addGenre(segment.substr(first, (last - first + 1)));
                    }
                }

                // Rebuild graph to update connections/visuals
                m_GraphManager->updateConnections();
                PushHistoryAction(HistoryAction{ HistoryActionType::EditBook, id, beforeEdit, *book });
                //m_LayoutDirty = true;
                //m_SettleIterations = 0;
                m_UIManager->ShowNotification("Book '" + title + "' updated!");
                m_HasUnsavedChanges = true;
            }
        },
        [this]() {
            if (m_GraphManager) {
                if (m_GraphManager->getLayoutMode() == LayoutMode::Physics) {
                    m_GraphManager->setLayoutMode(LayoutMode::Grid);
                }
                else {
                    m_GraphManager->setLayoutMode(LayoutMode::Physics);
                }
            }
        },
        [this](Status s) {
			if (m_GraphManager) {
				m_GraphManager->toggleStatusVisibility(s);
			}
            
        },
        [this]() {
            return GetReadBooksCount();
        },
        [this]() {
            return m_ReadingGoalTarget;
        },
        [this]() {
            return GetReadingGoalProgress();
        },
        [this](int delta) {
            AdjustReadingGoalTarget(delta);
            m_UIManager->ShowNotification("Reading goal updated.");
        },
        [this]() {
            ResetReadingGoalProgressBaseline();
            m_UIManager->ShowNotification("Reading goal progress reset.");
        },
        [this](int sortMode) {
            BookSortMode mode = BookSortMode::IdAsc;
            if (sortMode == 1) mode = BookSortMode::AuthorAsc;
            else if (sortMode == 2) mode = BookSortMode::RatingDesc;
            else if (sortMode == 3) mode = BookSortMode::DateAddedDesc;

            m_BookManager.sortBooks(mode);
            if (m_GraphManager) {
                m_GraphManager->initializePositions();
            }
            m_HasUnsavedChanges = true;
        }
    
    );
	
    
    SetTargetFPS(60);
}

void Application::Run()
{
    bool exitApp = false;

    while (!exitApp)
    {
       
        if (WindowShouldClose())
        {
           
            if (!m_BookManager.getBooks().empty()) {
             
                if (m_HasUnsavedChanges)
                {
                    int result = tinyfd_messageBox(
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

    // Destroy objects that may own raylib resources before closing the window/context.
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

void Application::LoadConfig()
{
    std::ifstream configFile(".nooklink_config");
    if (configFile.is_open()) {
        std::string line;
        bool hasStructuredData = false;
        while (std::getline(configFile, line)) {
            if (line.empty()) continue;

            const size_t eqPos = line.find('=');
            if (eqPos == std::string::npos) {
                if (!hasStructuredData && fs::exists(line)) {
                    m_SaveFileName = line;
                    Log::Info("Restored last session file: " + m_SaveFileName.string());
                }
                continue;
            }

            hasStructuredData = true;
            const std::string key = line.substr(0, eqPos);
            const std::string value = line.substr(eqPos + 1);

            if (key == "save_path") {
                if (fs::exists(value)) {
                    m_SaveFileName = value;
                    Log::Info("Restored last session file: " + m_SaveFileName.string());
                }
            }
            else if (key == "goal_target") {
                try {
                    m_ReadingGoalTarget = std::max(1, std::stoi(value));
                }
                catch (...) {}
            }
            else if (key == "goal_baseline") {
                try {
                    m_ReadingGoalBaselineRead = std::max(0, std::stoi(value));
                }
                catch (...) {}
            }
        }
        configFile.close();
    }
}

void Application::SaveConfig()
{
    std::ofstream configFile(".nooklink_config");
    if (configFile.is_open()) {
        configFile << "save_path=" << m_SaveFileName.string() << "\n";
        configFile << "goal_target=" << m_ReadingGoalTarget << "\n";
        configFile << "goal_baseline=" << m_ReadingGoalBaselineRead << "\n";
        configFile.close();
    }
}

int Application::GetReadBooksCount() const
{
    int readCount = 0;
    for (const auto& book : m_BookManager.getBooks()) {
        if (book.getStatus() == Status::Read) {
            ++readCount;
        }
    }
    return readCount;
}

void Application::AdjustReadingGoalTarget(int delta)
{
    m_ReadingGoalTarget = std::clamp(m_ReadingGoalTarget + delta, 1, 10000);
    SaveConfig();
}

int Application::GetReadingGoalProgress() const
{
    return std::max(0, GetReadBooksCount() - m_ReadingGoalBaselineRead);
}

void Application::ResetReadingGoalProgressBaseline()
{
    m_ReadingGoalBaselineRead = GetReadBooksCount();
    SaveConfig();
}

void Application::UpdateStartScreen()
{
    if (m_BtnNewGraph) m_BtnNewGraph->Update();
    if (m_BtnLoadGraph) m_BtnLoadGraph->Update();

    if (m_BtnContinue) {
        // Optional: Only enable/show button if data exists or file exists
        bool canContinue = !m_BookManager.getBooks().empty() || fs::exists(m_SaveFileName);

        // If you implemented an 'isVisible' or 'isEnabled' flag in Button, set it here.
        // For now, we just update it normally:
        if (canContinue) m_BtnContinue->Update();
    }
}

void Application::DrawStartScreen()
{
    // Draw Title
    const char* title = "NookLink";
    int fontSize = 64;
    int titleWidth = MeasureText(title, fontSize);
    m_TextRenderer->DrawTextCentered(title, { m_ScreenSize.x / 2.0f, m_ScreenSize.y / 4.0f }, fontSize, RAYWHITE);

    // Draw Buttons
    if (m_BtnNewGraph) m_BtnNewGraph->Draw(m_TextRenderer.get());
    if (m_BtnLoadGraph) m_BtnLoadGraph->Draw(m_TextRenderer.get());

    bool canContinue = !m_BookManager.getBooks().empty() || fs::exists(m_SaveFileName);

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

    
    // Determine visible area
    Rectangle viewRect = m_GraphManager->getCameraViewRect(m_CameraHandler->getCamera(), m_ScreenSize);

    const double edgesStart = NowSeconds();
    m_GraphManager->drawEdges(m_CameraHandler->getCamera().zoom, viewRect);
    m_ProfileCurrent.drawEdgesMs = (float)((NowSeconds() - edgesStart) * 1000.0);

    const double nodesStart = NowSeconds();
    m_GraphManager->drawNodes(m_CameraHandler->getCamera().zoom, viewRect);
    m_ProfileCurrent.drawNodesMs = (float)((NowSeconds() - nodesStart) * 1000.0);

    m_CameraHandler->endMode();

    const double uiStart = NowSeconds();
    m_UIManager->Draw(GetMousePosition(),m_GraphManager.get(),m_BookManager, m_TextRenderer.get());
    m_ProfileCurrent.drawUiMs = (float)((NowSeconds() - uiStart) * 1000.0);

    m_ProfileCurrent.frameCpuMs = (float)((NowSeconds() - frameCpuStart) * 1000.0);
    SmoothProfile();

    if (m_ShowProfilingOverlay) {
        DrawProfilingOverlay();
    }

    }
    EndDrawing();
}





void Application::Update()
{
    if (IsKeyPressed(KEY_F3)) {
        m_ShowProfilingOverlay = !m_ShowProfilingOverlay;
        Log::Info(std::string("Profiling overlay ") + (m_ShowProfilingOverlay ? "enabled" : "disabled"));
    }


    if (IsWindowResized()) {
        // Z�sk�me novou velikost
        m_ScreenSize.x = (float)GetScreenWidth();
        m_ScreenSize.y = (float)GetScreenHeight();

        // 1. Zaktualizujeme kameru (aby se n�m neposunul st�ed grafu)
        m_CameraHandler->updateScreenSize(m_ScreenSize);

        // 2. P�epo��t�me tla��tka na startovn� obrazovce
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


        Vector2 worldMousePos = GetScreenToWorld2D(GetMousePosition(), m_CameraHandler->getCamera());

        const bool ctrlDown = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
        if (ctrlDown && IsKeyPressed(KEY_Z)) {
            UndoLastAction();
        }
        else if (ctrlDown && IsKeyPressed(KEY_Y)) {
            RedoLastAction();
        }




        // Check for user interaction
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
            const float dt = std::clamp(GetFrameTime(), 0.0f, 0.05f);

            const double physicsStart = NowSeconds();
            m_GraphManager->updatePhysics(dt);
            m_ProfileCurrent.updatePhysicsMs = (float)((NowSeconds() - physicsStart) * 1000.0);
        }

        m_UIManager->Update(
            worldMousePos,
            GetMousePosition(),
            m_BookManager,
            m_GraphManager.get(), m_TextRenderer.get());

        if (!m_UIManager->IsBlockingGraphInteraction())
        {
            bool consumedInput = false;

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && IsKeyDown(KEY_LEFT_SHIFT)) {
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
                        Log::Info("Deleted book ID: " + std::to_string(clickedNode->id));
                        consumedInput = true;
                    }
                }
            }

            m_CameraHandler->update();
            if (!consumedInput) {
                m_InputHandler->ProcessInputs(worldMousePos, GetTime(), m_LayoutDirty, m_HasUnsavedChanges);
            }
            m_DebugManager->HandleDebugInputs(m_LayoutDirty);
        }
    }
}

void Application::SmoothProfile()
{
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

    const int x = 12;
    const int y = 320;
    const int w = 360;
    const int h = 174;
    const int fs = 20;
    const int lineStep = 24;

    DrawRectangleRounded({ (float)x, (float)y, (float)w, (float)h }, 0.15f, 10, Fade(BLACK, 0.65f));
    DrawRectangleRoundedLinesEx({ (float)x, (float)y, (float)w, (float)h }, 0.15f, 10, 2.0f, Fade(NookCol::UI_BORDER_SOFT, 0.85f));

    const float fps = GetFPS() > 0 ? (float)GetFPS() : 0.0f;
    const float frameBudgetMs = fps > 0.0f ? (1000.0f / fps) : 0.0f;

    DrawText(TextFormat("Profiling (F3)"), x + 12, y + 10, fs, RAYWHITE);
    DrawText(TextFormat("physics:   %6.2f ms", m_ProfileSmooth.updatePhysicsMs), x + 12, y + 10 + lineStep * 1, fs, RAYWHITE);
    DrawText(TextFormat("drawEdges: %6.2f ms", m_ProfileSmooth.drawEdgesMs), x + 12, y + 10 + lineStep * 2, fs, RAYWHITE);
    DrawText(TextFormat("drawNodes: %6.2f ms", m_ProfileSmooth.drawNodesMs), x + 12, y + 10 + lineStep * 3, fs, RAYWHITE);
    DrawText(TextFormat("drawUI:    %6.2f ms", m_ProfileSmooth.drawUiMs), x + 12, y + 10 + lineStep * 4, fs, RAYWHITE);
    DrawText(TextFormat("frameCPU:  %6.2f ms", m_ProfileSmooth.frameCpuMs), x + 12, y + 10 + lineStep * 5, fs, NookCol::UI_ACCENT_SOFT);
    DrawText(TextFormat("fps: %3i | budget: %5.2f ms", GetFPS(), frameBudgetMs), x + 12, y + 10 + lineStep * 6, fs, NookCol::UI_TEXT_MUTED);
    DrawText(TextFormat("nodes: %zu | edges: %zu", m_GraphManager->getNodes().size(), m_GraphManager->getEdgeCount()), x + 12, y + 10 + lineStep * 7, fs, NookCol::UI_TEXT_MUTED);
}


