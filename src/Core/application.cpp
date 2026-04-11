

// Implementation of the main Application class.
// Handles initialization, main loop, event processing, and shutdown.
// Manages undo/redo history and coordinates all major subsystems.

#define _CRT_SECURE_NO_WARNINGS
#include "application.h"
#include "UI/button.h"
#include "goodreadsCsvImporter.h"
#include "logging.h"

#include <tinyfiledialogs/tinyfiledialogs.h>

#include <filesystem>
#include <cstdlib> 
#include <algorithm>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fstream>
#include "colors.h"

namespace fs = std::filesystem;

namespace {
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
    // Keep history bounded to avoid unbounded memory growth on long sessions.
    if (m_UndoHistory.size() >= m_MaxHistoryEntries) {
        m_UndoHistory.erase(m_UndoHistory.begin());
    }
    m_UndoHistory.push_back(action);
    // Any new user action invalidates the forward redo branch.
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
    book->setPageCount(snapshot.getPageCount());
    book->setDatePublished(snapshot.getDatePublished());
    book->setRating(snapshot.getRating());
    book->setNotes(snapshot.getNotes());
    book->setDateAdded(snapshot.getDateAdded());
    book->setDateStartedReading(snapshot.getDateStartedReading());
    book->setDateFinishedReading(snapshot.getDateFinishedReading());
    // Genres are rebuilt from snapshot to preserve exact ordering/content.
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
    else if (action.type == HistoryActionType::BulkEdit) {
        success = true;
        for (const Book& snapshot : action.beforeBatch) {
            if (!ApplyBookSnapshot(snapshot)) {
                success = false;
            }
        }
        if (success) {
            m_GraphManager->initializePositions(true);
        }
    }

    if (!success) {
        m_UIManager->ShowNotification("Undo failed.");
        Log::Warn("Undo failed");
        return false;
    }

    // Store the same action in redo stack so we can re-apply it later.
    if (m_RedoHistory.size() >= m_MaxHistoryEntries) {
        m_RedoHistory.erase(m_RedoHistory.begin());
    }
    m_RedoHistory.push_back(action);

    if (action.type == HistoryActionType::BulkEdit) {
        const bool shouldWakePhysics = m_GraphManager && m_GraphManager->HasSignificantNodeOverlaps();
        if (m_GraphManager && !shouldWakePhysics) {
            m_GraphManager->setPhysicsActive(false);
        }
        m_LayoutDirty = shouldWakePhysics;
    }
    else {
        m_LayoutDirty = true;
    }
    m_HasUnsavedChanges = true;
    m_SettleIterations = 0;
    m_UIManager->MarkAnalyticsDirty();
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
    else if (action.type == HistoryActionType::BulkEdit) {
        success = true;
        for (const Book& snapshot : action.afterBatch) {
            if (!ApplyBookSnapshot(snapshot)) {
                success = false;
            }
        }
        if (success) {
            m_GraphManager->initializePositions(true);
        }
    }

    if (!success) {
        m_UIManager->ShowNotification("Redo failed.");
        Log::Warn("Redo failed");
        return false;
    }

    // After successful redo, action returns to undo stack.
    if (m_UndoHistory.size() >= m_MaxHistoryEntries) {
        m_UndoHistory.erase(m_UndoHistory.begin());
    }
    m_UndoHistory.push_back(action);

    if (action.type == HistoryActionType::BulkEdit) {
        const bool shouldWakePhysics = m_GraphManager && m_GraphManager->HasSignificantNodeOverlaps();
        if (m_GraphManager && !shouldWakePhysics) {
            m_GraphManager->setPhysicsActive(false);
        }
        m_LayoutDirty = shouldWakePhysics;
    }
    else {
        m_LayoutDirty = true;
    }
    m_HasUnsavedChanges = true;
    m_SettleIterations = 0;
    m_UIManager->MarkAnalyticsDirty();
    m_UIManager->ShowNotification("Redo applied.");
    Log::Info("Redo applied");
    return true;
}

Application::Application()
{
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
        // Keep Save As default path user-friendly across platforms.
        return fs::path(homeDir) / "Desktop";
    }

    // Fallback: return current working directory or empty path
    return fs::current_path();
}

void Application::ImportGoodreadsCsv()
{
    const char* filters[] = { "*.csv" };
    fs::path initialPath = getDesktopPath();
    if (!fs::exists(initialPath)) {
        initialPath = fs::current_path();
    }

    const char* selection = tinyfd_openFileDialog(
        "Import Goodreads CSV",
        initialPath.string().c_str(),
        1,
        filters,
        "CSV Files",
        0
    );

    if (!selection) {
        Log::Info("Goodreads CSV import cancelled");
        return;
    }

    GoodreadsCsvImporter importer;
    std::vector<Book> importedBooks;
    std::string importError;
    if (!importer.ImportFromFile(selection, importedBooks, importError)) {
        const std::string message = "CSV import failed: " + importError;
        m_UIManager->ShowNotification(message);
        Log::Warn(message);
        return;
    }

    int importedCount = 0;
    for (const Book& book : importedBooks) {
        m_BookManager.addBook(book);
        ++importedCount;
    }

    if (importedCount <= 0) {
        m_UIManager->ShowNotification("CSV import failed: no books imported.");
        Log::Warn("CSV import produced zero books");
        return;
    }

    ClearHistory();
    m_GraphManager->initializePositions();
    m_LayoutDirty = true;
    m_SettleIterations = 0;
    m_UIManager->MarkAnalyticsDirty();
    m_HasUnsavedChanges = true;
    m_UIManager->ShowNotification("Imported " + std::to_string(importedCount) + " books. Tip: use filter g:none to fill genres.");
    Log::Info("Imported " + std::to_string(importedCount) + " books from Goodreads CSV");
}

void Application::ExportLibraryCsv()
{
    const char* filters[] = { "*.csv" };

    fs::path defaultSavePath = getDesktopPath() / "library_export.csv";
    const char* path = tinyfd_saveFileDialog(
        "Export Library CSV...",
        defaultSavePath.string().c_str(),
        1,
        filters,
        "CSV Files"
    );

    if (!path) {
        Log::Info("CSV export cancelled");
        return;
    }

    auto CsvEscape = [](const std::string& value) {
        std::string escaped = value;
        size_t pos = 0;
        while ((pos = escaped.find('"', pos)) != std::string::npos) {
            escaped.insert(pos, 1, '"');
            pos += 2;
        }
        return std::string("\"") + escaped + "\"";
    };

    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) {
        m_UIManager->ShowNotification("CSV export failed: cannot open file.");
        Log::Warn("CSV export failed: cannot open file");
        return;
    }

    out << "Id,Title,Author,Status,Genres,Pages,Published,Rating,Date Added,Date Started,Date Finished,Notes\n";

    for (const Book& b : m_BookManager.getBooks()) {
        std::string genresJoined;
        const auto& genres = b.getGenres();
        for (size_t i = 0; i < genres.size(); ++i) {
            if (i > 0) genresJoined += "; ";
            genresJoined += genres[i];
        }

        out
            << b.getId() << ","
            << CsvEscape(b.getTitle()) << ","
            << CsvEscape(b.getAuthor()) << ","
            << CsvEscape(statusToString(b.getStatus())) << ","
            << CsvEscape(genresJoined) << ","
            << b.getPageCount() << ","
            << CsvEscape(b.getDatePublished()) << ","
            << b.getRating() << ","
            << CsvEscape(b.getDateAdded()) << ","
            << CsvEscape(b.getDateStartedReading()) << ","
            << CsvEscape(b.getDateFinishedReading()) << ","
            << CsvEscape(b.getNotes())
            << "\n";
    }

    out.flush();
    if (!out) {
        m_UIManager->ShowNotification("CSV export failed while writing file.");
        Log::Warn("CSV export failed while writing");
        return;
    }

    m_UIManager->ShowNotification("CSV export finished.");
    Log::Info("CSV export finished: " + std::string(path));
}

void Application::SaveLibrary()
{
    if (m_SaveFileName.empty()) {
        m_UIManager->ShowNotification("Save failed: no target file selected.");
        Log::Warn("Save blocked: empty save file name");
        return;
    }

    Log::Info("Saving library to: " + m_SaveFileName.string());
    auto currentPositions = m_GraphManager->exportPositions();
    if (!m_BookManager.saveBooksToFile(m_SaveFileName.string(), currentPositions)) {
        m_UIManager->ShowNotification("Save failed. Check logs for details.");
        return;
    }

    m_UIManager->ShowNotification("Library Saved!");
    m_HasUnsavedChanges = false;
}

void Application::SaveLibraryAs()
{
    const char* filters[] = { "*.json" };

    fs::path defaultSavePath = getDesktopPath() / "library.json";
    const char* path = tinyfd_saveFileDialog(
        "Save Library As...",
        defaultSavePath.string().c_str(),
        1,
        filters,
        "JSON Files"
    );

    if (!path) {
        Log::Info("Save As cancelled");
        return;
    }

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

void Application::LoadLibraryFromCurrentFile()
{
    if (!fs::exists(m_SaveFileName)) {
        m_UIManager->ShowNotification("Load failed: file does not exist.");
        Log::Warn("Load blocked: missing file " + m_SaveFileName.string());
        return;
    }

    Log::Info("Loading books from: " + m_SaveFileName.string());

    std::unordered_map<int, NodePosition> loadedPositions;
    if (!m_BookManager.loadBooksFromFile(m_SaveFileName.string(), loadedPositions)) {
        m_UIManager->ShowNotification("Load failed. Check logs for details.");
        return;
    }

    m_GraphManager->clearGenresAndConnections();
    m_GraphManager->initializePositions();
    m_GraphManager->applyLoadedPositions(loadedPositions);

    m_LayoutDirty = true;
    SaveConfig();
    ClearHistory();
    m_UIManager->ShowNotification("Library Loaded!");
    m_UIManager->MarkAnalyticsDirty();
    m_HasUnsavedChanges = false;
    m_AppState = AppState::Editor;
}

void Application::ReturnToStartScreen()
{
    m_AppState = AppState::StartScreen;
    if (m_BtnContinue) {
        std::string btnText = "Continue";
        if (fs::exists(m_SaveFileName)) {
            btnText += " (" + m_SaveFileName.filename().string() + ")";
        }
        m_BtnContinue->SetText(btnText);
    }
}

void Application::Initialize()
{
    Log::Init();
    Log::Info("Application initialization started");

    SetConfigFlags(FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow((int)m_ScreenSize.x, (int)m_ScreenSize.y, "NookLink");

    SetExitKey(KEY_NULL);

    Image icon = LoadImage("assets/icon2.png");

    SetWindowIcon(icon);
    UnloadImage(icon);

    m_TextRenderer = std::make_unique<TextRenderer>();


    m_GraphManager = std::make_unique<GraphManager>(m_BookManager, m_ConnectionManager, m_CanvasSize, m_TextRenderer.get());
    m_GraphManager->setLayoutDensityScale(m_LayoutDensityScale);
    m_CameraHandler = std::make_unique<CameraHandler>(m_ScreenSize, m_CanvasSize);

    m_UIManager = std::make_unique<UIManager>((int)m_ScreenSize.x, (int)m_ScreenSize.y);

    m_InputHandler = std::make_unique<InputHandler>(m_GraphManager.get(), m_BookManager, m_UIManager.get());
    m_DebugManager = std::make_unique<DebugManager>(m_BookManager);
   
    float centerX = m_ScreenSize.x / 2.0f;
    float centerY = m_ScreenSize.y / 2.0f;


    std::string btnText = "Continue";
    if (fs::exists(m_SaveFileName)) {
        btnText += " (" + m_SaveFileName.filename().string() + ")";
    }

    m_BtnContinue = std::make_shared<Button>(
        Anchor::Center,           
        Vector2{ 0, -140 },  
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

            // Rebuild graph structures first, then overlay persisted node coordinates.
            m_GraphManager->clearGenresAndConnections();
            m_GraphManager->initializePositions();

            m_GraphManager->applyLoadedPositions(loadedPositions);

            // Keep restored coordinates stable; nodes without saved position keep their initializePositions() spawn.
            m_GraphManager->setPhysicsActive(false);
            m_LayoutDirty = false;
            ClearHistory();
            m_UIManager->MarkAnalyticsDirty();
            m_AppState = AppState::Editor;
        }
    );

    m_BtnNewGraph = std::make_shared<Button>(
        Anchor::Center,            
        Vector2{ 0, -60 },         
        Vector2{ 300, 50 },
        "Start New Graph",
        [this]() {
            Log::Info("Starting new graph");
          
            m_BookManager = BookManager(); 
            ClearHistory();

           

            m_GraphManager->initializePositions();
            m_LayoutDirty = true;
            m_UIManager->MarkAnalyticsDirty();
            m_AppState = AppState::Editor; // Switch State
        }
    );


    m_BtnLoadGraph = std::make_shared<Button>(
        Anchor::Center,            
        Vector2{ 0, 20 },          
        Vector2{ 300, 50 },
        "Load From File",
        [this]() {
         
            const char* filters[] = { "*.json" };
            fs::path initialPath = getDesktopPath();

        
            if (!fs::exists(initialPath)) {
                initialPath = fs::current_path();
            }

           
            const char* selection = tinyfd_openFileDialog(
                "Select a Graph File",
                initialPath.string().c_str(),
                1,
                filters,
                "JSON Files",
                0
            );

          
            if (selection) {
                fs::path selectedPath(selection);

                Log::Info("User selected library file: " + selectedPath.string());

                m_SaveFileName = selectedPath;

              
                if (fs::exists(m_SaveFileName)) {
                   
                    std::unordered_map<int, NodePosition> loadedPositions;
                    if (!m_BookManager.loadBooksFromFile(m_SaveFileName.string(), loadedPositions)) {
                        m_UIManager->ShowNotification("Load failed. Check logs for details.");
                        return;
                    }

                    SaveConfig();

                 
                    m_GraphManager->clearGenresAndConnections();
                    m_GraphManager->initializePositions();

                   
                    m_GraphManager->applyLoadedPositions(loadedPositions);

                    // Keep restored coordinates stable; nodes without saved position keep their initializePositions() spawn.
                    m_GraphManager->setPhysicsActive(false);
                    m_LayoutDirty = false;
                    ClearHistory();
                    m_UIManager->MarkAnalyticsDirty();
                    m_AppState = AppState::Editor;
                }
                else {
                  
                    Log::Error("Selected file does not exist");
                    m_UIManager->ShowNotification("Selected file does not exist.");
                }
            }
            else {
                Log::Info("Load cancelled by user");
            }
        }
    );

  
   

    m_UIManager->BuildInterface(
        [this]() { // onSave
            SaveLibrary();
        },
        [this]() {
            SaveLibraryAs();
        },
        [this]() { // onLoad
            LoadLibraryFromCurrentFile();
        },
        [this]() {
            ImportGoodreadsCsv();
        },
        [this]() {
            ExportLibraryCsv();
        },
        [this]() { 
            ReturnToStartScreen();
		},
        [this]() { UndoLastAction(); },
        [this]() { RedoLastAction(); },
        [this](int selectedThemeIndex) -> std::string {
            NookCol::ApplyThemePresetByIndex(selectedThemeIndex);
            m_ThemePresetIndex = NookCol::GetCurrentThemeIndex();
            SaveConfig();
            return NookCol::GetCurrentThemeName();
        },
        []() -> int {
            return NookCol::GetCurrentThemeIndex();
        },
        []() -> int {
            return NookCol::GetThemeCount();
        },
        [](int index) -> std::string {
            return NookCol::GetThemeNameByIndex(index);
        },
        [this](float densityScale) {
            m_LayoutDensityScale = std::clamp(densityScale, 0.3f, 1.6f);
            if (m_GraphManager) {
                m_GraphManager->setLayoutDensityScale(m_LayoutDensityScale);
            }
            SaveConfig();
        },
        [this]() -> float {
            return m_LayoutDensityScale;
        },
        [this](std::string title, std::string author, std::string genreStr, int pageCount, std::string publishedDate, float rating, Status status, std::string notes, std::string startedReadingDate, std::string finishedReadingDate) {
            Log::Info("Adding book: " + title);

            // Status and dates must stay coherent (Reading/Read implies started date, Read implies finished date).
            NormalizeDatesForStatus(status, startedReadingDate, finishedReadingDate);

           
            Book newBook(title, author, status);
            newBook.setPageCount(pageCount);
            newBook.setDatePublished(publishedDate);
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
                // Store exact post-insert snapshot for redo.
                PushHistoryAction(HistoryAction{ HistoryActionType::AddBook, newId, Book(), *addedBook });
            }

            m_LayoutDirty = true;
            m_SettleIterations = 0;
            m_UIManager->ShowNotification("Book '" + title + "' added!");
            m_UIManager->MarkAnalyticsDirty();
            m_HasUnsavedChanges = true;
        },
        //Edit book
        [this](int id, std::string title, std::string author, std::string genreStr, int pageCount, std::string publishedDate, float rating, Status status, std::string notes, std::string startedReadingDate, std::string finishedReadingDate) {
            Log::Info("Editing book ID: " + std::to_string(id));
            NormalizeDatesForStatus(status, startedReadingDate, finishedReadingDate);
            Book* book = m_BookManager.getBookById(id);
            if (book) {
                const Book beforeEdit = *book;
              
                book->setTitle(title);
                book->setAuthor(author);
                book->setPageCount(pageCount);
                book->setDatePublished(publishedDate);
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
              
                m_UIManager->ShowNotification("Book '" + title + "' updated!");
                m_UIManager->MarkAnalyticsDirty();
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

