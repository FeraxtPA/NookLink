

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

   
    float centerX = m_ScreenSize.x / 2.0f;
    float centerY = m_ScreenSize.y / 2.0f;


    std::string btnText = "Continue";
    if (fs::exists(m_SaveFileName)) {
        btnText += " (" + m_SaveFileName.filename().string() + ")";
    }

    m_BtnContinue = std::make_shared<Button>(
        Rectangle{ centerX - 150, centerY - 140, 300, 50 },
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
        Rectangle{ centerX - 150, centerY - 60, 300, 50 },
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
        Rectangle{ centerX - 150, centerY + 20, 300, 50 },
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
    m_UIManager = std::make_unique<UIManager>(m_ScreenSize.x, m_ScreenSize.y);

    m_UIManager->BuildInterface(
		[this]() { // onSave
			std::cout << "Saving books to " << m_SaveFileName << "..." << std::endl;
			m_BookManager.saveBooksToFile(m_SaveFileName.string());
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

void Application::InitBookManager()
{
    Book book1("1984", "George Orwell", Status::ToRead);
    book1.addGenre("Dystopian");
    book1.addGenre("ScienceFiction");
    book1.addGenre("ClassicLiterature");
    book1.setRating(3.00f);

    Book book2("Brave New World", "Aldous Huxley", Status::ToRead);
    book2.addGenre("Dystopian");
    book2.addGenre("ScienceFiction");
    book2.setRating(0.00f);

    Book book3("The Hobbit", "J.R.R. Tolkien", Status::Read);
    book3.addGenre("Fantasy");
    book3.addGenre("Adventure");
    book3.setRating(4.50f);
    book3.setNotes("An epic fantasy adventure set in Middle-earth.\nA prequel to The Lord of the Rings.\n- Bilbo's journey begins in the Shire.\n- Encounters with trolls, goblins, and Smaug the dragon.\n- Themes of courage, friendship, and the hero's journey.");

    Book book4("The Name of the Wind", "Patrick Rothfuss", Status::Reading);
    book4.addGenre("Fantasy");
    book4.addGenre("DarkFantasy");
    book4.setRating(1.25f);

    Book book5("A Brief History of Time", "Stephen Hawking", Status::Read);
    book5.addGenre("Science");
    book5.setRating(2.74f); 

    Book book6("Dune", "Frank Herbert", Status::ToRead);
    book6.addGenre("ScienceFiction");
    book6.setRating(5.00f);

    Book book7("Foundation", "Isaac Asimov", Status::ToRead);
    book7.addGenre("ScienceFiction");
    book7.setRating(3.75f);

    Book book8("Mistborn: The Final Empire", "Brandon Sanderson", Status::Reading);
    book8.addGenre("Fantasy");
    book8.setRating(4.49f); 

    m_BookManager.addBook(book1);
    m_BookManager.addBook(book2);
    m_BookManager.addBook(book3);
    m_BookManager.addBook(book4);
    m_BookManager.addBook(book5);
    m_BookManager.addBook(book6); 
    m_BookManager.addBook(book7);
    m_BookManager.addBook(book8);
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




void Application::HandleInput(Vector2 worldMousePos)
{
    // Only handle input if the user is interacting
    if (m_IsUserInteracting)
    {

        // Handle dragging + shift drag to lock
        if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {

            for (auto& node : m_GraphManager->getNodes()) {
                float dist = Vector2Distance(worldMousePos, node.position);
                if (dist <= node.radius) {
                    node.isDragged = true;
                    if (IsKeyDown(KEY_LEFT_SHIFT)) {

                        if (node.type != NodeType::Genre)
                            node.locked = true;
                    }
                    m_GraphManager->setDraggedNode(&node);
                    m_LayoutDirty = true;
                    break;
                }
            }
        }

        
      


        // Release drag
        if (IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) {
            if (m_GraphManager->getDraggedNode()) {
                m_GraphManager->getDraggedNode()->isDragged = false;
                m_GraphManager->setDraggedNode(nullptr);
            }
        }

        // Handle dragging movement and setting position
        if (m_GraphManager->getDraggedNode() && IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {

            Node* dragged = m_GraphManager->getDraggedNode();
            dragged->position = worldMousePos;

            if (dragged->type == NodeType::Genre) {
                m_GraphManager->updateGenrePosition(dragged->id, worldMousePos);
            }
            m_LayoutDirty = true;
        }

        //Spawn books and genre for testing
        if (IsKeyPressed(KEY_M))
        {
            for (int i = 0; i < 50; i++)
            {

                Book newBook("New Book", "Author", Status::ToRead);
                newBook.addGenre("History");

                int newBookId = m_BookManager.addBook(newBook);
            }
            m_GraphManager->initializePositions();
            m_LayoutDirty = true;
        }

        //DISABLE/ENABLE FPS CAP
        if (IsKeyPressed(KEY_V))
        {
            SetTargetFPS(0);
        }
        if (IsKeyPressed(KEY_B))
        {
            SetTargetFPS(GetMonitorRefreshRate(0));
        }
        //Print books with to-read status on key press
       

        // Handle left click for selecting/removing/unlocking nodes
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {

            Camera2D cam = m_CameraHandler->getCamera();
            Vector2 worldPos = GetScreenToWorld2D(GetMousePosition(), cam);

            Node* clickedNode = m_GraphManager->getNodeAtPosition(worldPos);

            double currentTime = GetTime();

            if (clickedNode)
            {
                //Shift + click to remove book node
                if (IsKeyDown(KEY_LEFT_SHIFT) && clickedNode->type == NodeType::Book)
                {
                    std::cout << "Removing book node with ID: " << clickedNode->id << std::endl;
                    m_BookManager.removeBook(clickedNode->id);
                    m_GraphManager->removeNodeById(clickedNode->id);
                    m_LayoutDirty = true;
                    m_LastClickedNode = nullptr;
                    m_LastClickTime = 0.0;
                }
                //Double click to unlock node
                else if (clickedNode == m_LastClickedNode && (currentTime - m_LastClickTime) <= m_DoubleClickThreshold)
                {
                    if (clickedNode->locked)
                    {
                        clickedNode->locked = false;
                        std::cout << "Unlocked node ID: " << clickedNode->id << std::endl;
                        m_LastClickedNode = nullptr;
                        m_LastClickTime = 0.0;
                    }
                }
                // For debugging: print clicked node info
                else
                {
                    m_LastClickedNode = clickedNode;
                    m_LastClickTime = currentTime;

                    if (clickedNode->type == NodeType::Book) {
                        const Book* clickedBook = m_BookManager.findBookById(clickedNode->id);
                        if (clickedNode)
                            std::cout << "Clicked book: " << clickedBook->getTitle() << std::endl;
                    }
                    else if (clickedNode->type == NodeType::Genre) {
                        std::string genreName = m_GraphManager->getGenreNameByNodeId(clickedNode->id);
                        std::cout << "Clicked genre: " << genreName << std::endl;
                    }
                }
            }
            
        }
    }

    if (IsKeyPressed(KEY_E)) {
        if (m_LastClickedNode && m_LastClickedNode->type == NodeType::Book) {
            Book* bookToEdit = m_BookManager.getBookById(m_LastClickedNode->id);
            if (bookToEdit) {
                m_UIManager->OpenEditPanel(bookToEdit);
            }
        }
    }

  

    if (IsKeyPressed(KEY_T))
    {
        std::print("Books with 'To Read' status:\n");
       
       
        for (const auto& [i, book] : m_BookManager.getBooksToBeRead() | std::views::enumerate)
        {
            std::print("{} {}-{}\n", i + 1, book.getTitle(), book.getAuthor());
        }
    }

}

void Application::Update()
{
   
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

    // Update layout if dirty
    if (m_LayoutDirty) {
        m_UpdateInterval -= GetFrameTime();
        if (m_UpdateInterval <= 0.0f) {

            
            m_GraphManager->resolveNodeOverlaps(25.0f);
            m_UpdateInterval = m_UpdateIntervalInitial;
            m_SettleIterations++;

            if (m_SettleIterations > m_MaxSettleIterations) {
                m_LayoutDirty = false;
                m_SettleIterations = 0;
            }
        }
    }

    if (!m_UIManager->IsMouseOverUI())
    {
        m_CameraHandler->update();
        HandleInput(worldMousePos); 
    }
   

    // Update UI
    m_UIManager->Update(
        worldMousePos,
        GetMousePosition(),
        m_BookManager,
        m_GraphManager.get(), m_TextRenderer.get());
    }
}


