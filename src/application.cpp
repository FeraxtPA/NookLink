

#define _CRT_SECURE_NO_WARNINGS
#include "application.h"
#include "UI/button.h"

#include "../include/tinyfiledialogs/tinyfiledialogs.h"

#include <filesystem>
#include <cstdlib> 

#include <fstream>
#include <print>
#include <ranges>

namespace fs = std::filesystem;

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
        Anchor::Center,            // Kotva na støed obrazovky
        Vector2{ 0, -140 },        // Odsazení od støedu (0 na ose X, 140 pixelù nahoru na ose Y)
        Vector2{ 300, 50 },
        btnText, // Uses the dynamic label
        [this]() {
            // Case 1: In-memory session exists
            if (!m_BookManager.getBooks().empty()) {
                m_AppState = AppState::Editor;
                return;
            }

            // Case 2: Load from the dynamic m_SaveFileName
            if (fs::exists(m_SaveFileName)) {
                std::cout << "Loading " << m_SaveFileName << "..." << std::endl;
                m_BookManager.loadBooksFromFile(m_SaveFileName.string());

                m_GraphManager->clearGenresAndConnections();
                m_GraphManager->initializePositions();
                m_LayoutDirty = true;

                m_AppState = AppState::Editor;
            }
        }
    );

    m_BtnNewGraph = std::make_shared<Button>(
        Anchor::Center,            // Kotva na støed
        Vector2{ 0, -60 },         // Odsazení od støedu (-60 Y)
        Vector2{ 300, 50 },
        "Start New Graph",
        [this]() {
            std::cout << "Starting New Graph..." << std::endl;
            // Option A: Start completely empty
            m_BookManager = BookManager(); 

            // Option B: Load your demo data (1984, etc.)
            //InitBookManager();

            m_GraphManager->initializePositions();
            m_LayoutDirty = true;
            m_AppState = AppState::Editor; // Switch State
        }
    );


    m_BtnLoadGraph = std::make_shared<Button>(
        Anchor::Center,            // Kotva na støed
        Vector2{ 0, 20 },          // Odsazení od støedu (+20 Y)
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

                std::cout << "User selected: " << selectedPath << std::endl;

                m_SaveFileName = selectedPath; // Assuming m_SaveFileName is fs::path or std::string

                // 4. Perform the Load
                if (fs::exists(m_SaveFileName)) {
                    // Pass generic string to your loader
                    m_BookManager.loadBooksFromFile(m_SaveFileName.string());

                    SaveConfig();

                    m_GraphManager->clearGenresAndConnections();
                    m_GraphManager->initializePositions();
                    m_LayoutDirty = true;

                    m_AppState = AppState::Editor;
                }
                else {
                    // This branch is rarely hit if selection is not null, 
                    // but good for sanity checking.
                    std::cerr << "Error: File does not exist." << std::endl;
                }
            }
            else {
                std::cout << "Load cancelled by user." << std::endl;
            }
        }
    );

    //Needs to be initialized after font is loaded
   

    m_UIManager->BuildInterface(
		[this]() { // onSave
			std::cout << "Saving books to " << m_SaveFileName << "..." << std::endl;
			m_BookManager.saveBooksToFile(m_SaveFileName.string());
            m_UIManager->ShowNotification("Library Saved!");
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
                m_SaveFileName = path; // Update the app's current file path
                m_BookManager.saveBooksToFile(m_SaveFileName.string()); // Save immediately

                SaveConfig();

                std::cout << "Saved As: " << m_SaveFileName << std::endl;

                m_UIManager->ShowNotification("Saved As: " + m_SaveFileName.filename().string());
            }
            else {
                std::cout << "Save As cancelled." << std::endl;
            }
        },
		[this]() { // onLoad
			std::cout << "Loading books from " << m_SaveFileName << "..." << std::endl;
			m_BookManager.loadBooksFromFile(m_SaveFileName.string());
			m_GraphManager->clearGenresAndConnections();
			m_GraphManager->initializePositions();
			m_LayoutDirty = true;
            SaveConfig();
            m_UIManager->ShowNotification("Library Loaded!");
          
           
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
        [this](std::string title, std::string author, std::string genreStr, float rating, Status status, std::string notes) {
            std::cout << "Adding book: " << title << std::endl;


           
            Book newBook(title, author, status);
            newBook.setRating(rating);
            newBook.setNotes(notes);

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
            m_BookManager.addBook(newBook);
            m_GraphManager->initializePositions();
            m_LayoutDirty = true;
            m_SettleIterations = 0;
            m_UIManager->ShowNotification("Book '" + title + "' added!");
        },
        //Edit book
        [this](int id, std::string title, std::string author, std::string genreStr, float rating, Status status, std::string notes) {
            std::cout << "Editing book ID: " << id << std::endl;
            Book* book = m_BookManager.getBookById(id);
            if (book) {
              
                book->setTitle(title);
                book->setAuthor(author);
                book->setRating(rating);
                book->setStatus(status);
                book->setNotes(notes);

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
                //m_LayoutDirty = true;
                //m_SettleIterations = 0;
                m_UIManager->ShowNotification("Book '" + title + "' updated!");
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
        }
    );
	
    
    SetTargetFPS(60);
}

void Application::Run()
{
    while (!WindowShouldClose())
    {
        Update();
        Draw();
    }
}

void Application::Shutdown()
{
    if (!m_BookManager.getBooks().empty()) {
        std::cout << "Auto-saving session to " << m_SaveFileName << "..." << std::endl;
        m_BookManager.saveBooksToFile(m_SaveFileName.string());
    }

	CloseWindow();
}

void Application::LoadConfig()
{
    std::ifstream configFile(".nooklink_config");
    if (configFile.is_open()) {
        std::string pathStr;
        if (std::getline(configFile, pathStr) && !pathStr.empty()) {
            if (fs::exists(pathStr)) {
                m_SaveFileName = pathStr;
                std::cout << "Restored last session file: " << m_SaveFileName << std::endl;
            }
        }
        configFile.close();
    }
}

void Application::SaveConfig()
{
    std::ofstream configFile(".nooklink_config");
    if (configFile.is_open()) {
        configFile << m_SaveFileName.string();
        configFile.close();
    }
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
    
    m_GraphManager->drawEdges(m_CameraHandler->getCamera().zoom, viewRect);

 
    m_GraphManager->drawNodes(m_CameraHandler->getCamera().zoom, viewRect);

    m_CameraHandler->endMode();

   
    m_UIManager->Draw(GetMousePosition(),m_GraphManager.get(),m_BookManager, m_TextRenderer.get());

    }
    EndDrawing();
}





void Application::Update()
{

    if (IsWindowResized()) {
        // Získáme novou velikost
        m_ScreenSize.x = (float)GetScreenWidth();
        m_ScreenSize.y = (float)GetScreenHeight();

        // 1. Zaktualizujeme kameru (aby se nám neposunul støed grafu)
        m_CameraHandler->updateScreenSize(m_ScreenSize);

        // 2. Pøepoèítáme tlaèítka na startovní obrazovce
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
            m_GraphManager->updatePhysics(1.0f);
        }

        m_UIManager->Update(
            worldMousePos,
            GetMousePosition(),
            m_BookManager,
            m_GraphManager.get(), m_TextRenderer.get());

        if (!m_UIManager->IsMouseOverUI())
        {
            m_CameraHandler->update();
            m_InputHandler->ProcessInputs(worldMousePos, GetTime(), m_LayoutDirty);

            m_DebugManager->HandleDebugInputs(m_LayoutDirty);

         
           
        }
    }
}


