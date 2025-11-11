
#include <raylib.h>
#include "book.h"
#include "bookManager.h"
#include "connectionManager.h"
#include "graphManager.h"
#include <iostream>
#include "cameraHandler.h"
#include <format>



int main()
{
    const int screenWidth = 1920;
    const int screenHeight = 1000;



    SetConfigFlags(FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "NookLink");

    

    BookManager bookManager;

    // Create some books with genres
    Book book1("1984", "George Orwell", Status::Read);
    book1.addGenre(Genre::Dystopian);
    book1.addGenre(Genre::ScienceFiction);
    book1.addGenre(Genre::ClassicLiterature);
   
    book1.setRating(3.33f);
    Book book2("Brave New World", "Aldous Huxley", Status::ToRead);
    book2.addGenre(Genre::Dystopian);
    book2.addGenre(Genre::ScienceFiction);


    

    Book book3("The Hobbit", "J.R.R. Tolkien", Status::Read);
    book3.addGenre(Genre::Fantasy);
    book3.addGenre(Genre::Adventure);
   
    book3.setRating(4.5f);
    book3.setNotes("An epic fantasy adventure set in Middle-earth.\nA prequel to The Lord of the Rings.\n- Bilbo's journey begins in the Shire.\n- Encounters with trolls, goblins, and Smaug the dragon.\n- Themes of courage, friendship, and the hero's journey.");
    Book book4("The Name of the Wind", "Patrick Rothfuss", Status::Reading);
    book4.addGenre(Genre::Fantasy);
    book4.addGenre(Genre::DarkFantasy);

   

    bookManager.addBook(book1);
    bookManager.addBook(book2);
    bookManager.addBook(book3);
    bookManager.addBook(book4);


    Vector2 canvasSize = { 2000, 2000 };


    ConnectionManager connectionManager;
    //connectionManager.updateConnections(bookManager.getBooks());

    GraphManager graphRenderer(bookManager, connectionManager, canvasSize);
    graphRenderer.initializePositions();

   

    CameraHandler cameraHandler(screenWidth, screenHeight, canvasSize);

    //SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));
    SetTargetFPS(60);

    Node* lastClickedNode = nullptr;
    double lastClickTime = 0.0;
    const double doubleClickThreshold = 0.3;

    float updateInterval = 0.01f; // Update every 10ms
    bool layoutDirty = true;

    static int settleIterations = 0;
    const int maxSettleIterations = 1000;



    static Node* lastHoveredNode = nullptr;
    static double hoverStartTime = 0.0;
    static std::string cachedTooltipText;
    static std::vector<std::string> cachedLines;
    static int cachedBoxWidth = 0;
    static int cachedBoxHeight = 0;
    static Vector2 lastMousePos = { -1, -1 };

    bool userIsInteracting = false;
   
    int codepoints[97] = { 0 };
    for (int i = 0; i < 95; i++) codepoints[i] = 32 + i;
    codepoints[95] = 0x00BD; //  1/2
    codepoints[96] = 0x2605; // star
    

    Font font = LoadFontEx("Assets/DejaVuSans.ttf", 20, codepoints, 97);

    SetTextureFilter(font.texture, RL_TEXTURE_FILTER_POINT);

    const char* saveFileName = "my_books.json";
    Rectangle saveButton = { (float)screenWidth - 220, 10, 100, 40 };
    Rectangle loadButton = { (float)screenWidth - 110, 10, 100, 40 };

    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();


        Vector2 mousePos = GetMousePosition();
        Vector2 worldMousePos = GetScreenToWorld2D(mousePos, cameraHandler.getCamera());

        cameraHandler.update();
        Rectangle viewRect = graphRenderer.getCameraViewRect(cameraHandler.getCamera(), screenWidth, screenHeight);

        if (IsWindowFocused())
        {
            userIsInteracting =
                IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) ||
                IsMouseButtonReleased(MOUSE_RIGHT_BUTTON) ||
                IsMouseButtonDown(MOUSE_RIGHT_BUTTON) ||
                IsMouseButtonPressed(MOUSE_LEFT_BUTTON) ||
                IsKeyPressed(KEY_SPACE) ||
                IsKeyPressed(KEY_V) ||
                IsKeyPressed(KEY_B);
        }
    
        if (!IsMouseButtonDown(MOUSE_RIGHT_BUTTON) && !graphRenderer.getDraggedNode())
        {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                // Zkontrolovat kliknutí na tlačítko ULOŽIT
                if (CheckCollisionPointRec(mousePos, saveButton))
                {
                    std::cout << "Saving books to " << saveFileName << "..." << std::endl;
                    bookManager.saveBooksToFile(saveFileName);
                }

                // Zkontrolovat kliknutí na tlačítko NAČÍST
                else if (CheckCollisionPointRec(mousePos, loadButton))
                {
                    std::cout << "Loading books from " << saveFileName << "..." << std::endl;
                    bookManager.loadBooksFromFile(saveFileName);

                    // DŮLEŽITÉ: Po načtení musíme znovu sestavit graf
                    graphRenderer.initializePositions(); // Znovu vytvoří uzly z bookManageru
                    layoutDirty = true;                  // Spustí nové usazení grafu
                    settleIterations = 0;                // Resetuje počítadlo usazení
                }
            }
        }

        if (layoutDirty) {
            updateInterval -= deltaTime;
            if (updateInterval <= 0.0f) {
               
                graphRenderer.resolveNodeOverlaps(25.0f);
                updateInterval = 0.01f;
                settleIterations++;

                if (settleIterations > maxSettleIterations) {
                    layoutDirty = false;
                    settleIterations = 0;
                }
            }
        }


        if (userIsInteracting)
        {

       
        if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {


            for (auto& node : graphRenderer.getNodes()) {
                float dist = Vector2Distance(worldMousePos, node.position);
                if (dist <= node.radius) {
                    node.isDragged = true;
                    if (IsKeyDown(KEY_LEFT_SHIFT)) {

                        if (node.type != NodeType::Genre)
                            node.locked = true;
                    }
                    graphRenderer.setDraggedNode(&node);
                    layoutDirty = true;
                    break;
                }
            }
        }


        if (IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) {
            if (graphRenderer.getDraggedNode()) {
                graphRenderer.getDraggedNode()->isDragged = false;
                graphRenderer.setDraggedNode(nullptr);
            }
        }

        if (graphRenderer.getDraggedNode() && IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {

  

            Node* dragged = graphRenderer.getDraggedNode();
            dragged->position = worldMousePos;

            if (dragged->type == NodeType::Genre) {
                graphRenderer.updateGenrePosition(dragged->id, worldMousePos);
            }
            layoutDirty = true;
        }




        if (IsKeyPressed(KEY_SPACE))
        {


            for (int i = 0; i < 50; i++)
            {

                Book newBook("New Book", "Author", Status::ToRead);
                newBook.addGenre(Genre::History);

                int newBookId = bookManager.addBook(newBook);
            }

            graphRenderer.initializePositions();
            layoutDirty = true;
        }


        if (IsKeyPressed(KEY_V))
        {
            SetTargetFPS(0);
        }
        if(IsKeyPressed(KEY_B))
		{
			SetTargetFPS(GetMonitorRefreshRate(0));
		}

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
         
            Camera2D cam = cameraHandler.getCamera();
            Vector2 worldPos = GetScreenToWorld2D(mousePos, cam);

            Node* clickedNode = graphRenderer.getNodeAtPosition(worldPos);

            double currentTime = GetTime();

            if (clickedNode)
            {
                if (IsKeyDown(KEY_LEFT_SHIFT) && clickedNode->type == NodeType::Book)
                {
                    std::cout << "Removing book node with ID: " << clickedNode->id << std::endl;
                    bookManager.removeBook(clickedNode->id); 
                    graphRenderer.removeNodeById(clickedNode->id); 
                    layoutDirty = true;
                    lastClickedNode = nullptr;
                    lastClickTime = 0.0;
                }
                else if (clickedNode == lastClickedNode && (currentTime - lastClickTime) <= doubleClickThreshold)
                {
                    if (clickedNode->locked)
                    {
                        clickedNode->locked = false;
                        std::cout << "Unlocked node ID: " << clickedNode->id << std::endl;
                        lastClickedNode = nullptr;
                        lastClickTime = 0.0;
                    }
                }
                else
                {
                    lastClickedNode = clickedNode;
                    lastClickTime = currentTime;

                    if (clickedNode->type == NodeType::Book) {
                        const Book* clickedBook = bookManager.findBookById(clickedNode->id);
                        if(clickedNode)
                            std::cout << "Clicked book: " << clickedBook->getTitle() << std::endl;
                    }
                    else if (clickedNode->type == NodeType::Genre) {
                        std::string genreName = graphRenderer.getGenreNameByNodeId(clickedNode->id);
                        std::cout << "Clicked genre: " << genreName << std::endl;
                    }
                }
            }

            else
            {
                std::cout << "Clicked on empty space" << std::endl;
                lastClickedNode = nullptr;
                lastClickTime = 0.0;
            }
        }
        }


        BeginDrawing();
        ClearBackground(NookCol::BACKGROUND);



    

        cameraHandler.beginMode();

        graphRenderer.drawEdges(cameraHandler.getCamera().zoom, viewRect);


        for (const auto& node : graphRenderer.getNodes()) {
            if (graphRenderer.isNodeVisible(node, viewRect)) {
                graphRenderer.drawNode(node, cameraHandler.getCamera().zoom);
            }
        }




        cameraHandler.endMode();

        // === PŘIDAT TENTO KÓD PRO VYKRESLENÍ ===

        // Vykreslit tlačítko ULOŽIT
        bool saveHover = CheckCollisionPointRec(mousePos, saveButton);
        DrawRectangleRec(saveButton, saveHover ? DARKGRAY : LIGHTGRAY);
        DrawText("Save", (int)(saveButton.x + saveButton.width / 2 - static_cast<float>(MeasureText("Save", 20)) / 2), (int)(saveButton.y + 10), 20, BLACK);

        // Vykreslit tlačítko NAČÍST
        bool loadHover = CheckCollisionPointRec(mousePos, loadButton);
        DrawRectangleRec(loadButton, loadHover ? DARKGRAY : LIGHTGRAY);
        DrawText("Load", (int)(loadButton.x + loadButton.width / 2 - static_cast<float>(MeasureText("Load", 20)) / 2), (int)(loadButton.y + 10), 20, BLACK);

        // === KONEC PŘIDANÉHO KÓDU ===

        DrawText("Right-click and drag to move nodes", 10, 10, 20, BLACK);
        DrawText("Press SPACE to add 10 new books", 10, 40, 20, BLACK);
        DrawText("Hold SHIFT while dragging to lock a node", 10, 70, 20, BLACK);
        DrawText("Middle click to pan around", 10, 100, 20, BLACK);
        DrawText("Double click a node to unlock it", 10, 130, 20, BLACK);
        DrawText("Press V to unlock framerate", 10, 160, 20, BLACK);
        DrawText("Press B to enable VSYNC", 10, 190, 20, BLACK);
        DrawText( (std::format("Zoom: {}",  cameraHandler.getCamera().zoom)).c_str(),  10, 220, 20, BLACK);

        std::string nodeAmountText = "Node amount: " + std::to_string(graphRenderer.getNodes().size());
        DrawText(nodeAmountText.c_str(), 10, screenHeight - 40, 20, BLACK);
        DrawFPS(10, screenHeight - 20);


        bool mouseMoved = (mousePos.x != lastMousePos.x || mousePos.y != lastMousePos.y);
        lastMousePos = mousePos;

        if (mouseMoved || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            Node* newlyHovered = graphRenderer.getNodeAtPosition(worldMousePos);
            if (newlyHovered != lastHoveredNode || IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
                lastHoveredNode = newlyHovered;
                hoverStartTime = GetTime();
                cachedTooltipText.clear(); // Force rebuild on node change
            }
        }

        if (lastHoveredNode && (GetTime() - hoverStartTime >= 0.5) && !IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            if (cachedTooltipText.empty()) {
                if (lastHoveredNode->type == NodeType::Book) {
                    const Book* hoveredBook = bookManager.findBookById(lastHoveredNode->id);
                    if (!hoveredBook) {
						cachedTooltipText = "Book not found!";
						cachedLines.clear();
						cachedBoxWidth = 0;
						cachedBoxHeight = 0;
						hoverStartTime = 0.0;
						lastHoveredNode = nullptr;
						continue;
					}
                    cachedTooltipText = "Title: " + hoveredBook->getTitle() +
                        "\nAuthor: " + hoveredBook->getAuthor() +
                        "\nCurrent Status:" + statusToString(hoveredBook->getStatus()) +
                        "\nNotes: " + hoveredBook->getNotes() +
                        "\nRating: " + Book::ratingToStars(hoveredBook->getRating()) +
                        "\nBookID: " + std::to_string(hoveredBook->getId());
                }
                else if (lastHoveredNode->type == NodeType::Genre) {
                    int bookConnectionCount = 0;
                    for (const auto& edge : graphRenderer.getEdges()) {
                        if ((edge.fromId == lastHoveredNode->id && graphRenderer.getNodeById(edge.toId)->type == NodeType::Book) ||
                            (edge.toId == lastHoveredNode->id && graphRenderer.getNodeById(edge.fromId)->type == NodeType::Book)) {
                            bookConnectionCount++;
                        }
                    }

                    cachedTooltipText = "Genre: " + graphRenderer.getGenreNameByNodeId(lastHoveredNode->id) +
                        "\nBooks: " + std::to_string(bookConnectionCount) +
                        "\nGenreID: " + std::to_string(lastHoveredNode->id);
                }

                // Layout
                const int fontSize = 20;
                const int padding = 16;
                const int lineSpacing = 12;

                cachedLines.clear();
                size_t start = 0, end;
                while ((end = cachedTooltipText.find('\n', start)) != std::string::npos) {
                    cachedLines.push_back(cachedTooltipText.substr(start, end - start));
                    start = end + 1;
                }
                cachedLines.push_back(cachedTooltipText.substr(start));

                int maxLineWidth = 0;
                for (const std::string& line : cachedLines) {
                    int width = MeasureText(line.c_str(), fontSize);
                    if (width > maxLineWidth) maxLineWidth = width;
                }

                cachedBoxWidth = maxLineWidth + 2 * padding;
                cachedBoxHeight = (fontSize + lineSpacing) * static_cast<int>(cachedLines.size()) - lineSpacing + 2 * padding;
            }

            // Draw the tooltip
            DrawRectangleRounded({ mousePos.x, mousePos.y, (float)cachedBoxWidth + 20, (float)cachedBoxHeight + 20 }, 0.2f, 10, Fade(NookCol::POPUP_BORDER, 0.75f));
            DrawRectangleRounded({ mousePos.x, mousePos.y, (float)cachedBoxWidth + 16, (float)cachedBoxHeight + 16 }, 0.2f, 10, Fade( NookCol::POPUP_BG , 0.95f));

            const int fontSize = 20;
            const int padding = 16;
            const int lineSpacing = 12;
            int yOffset = 0;
            for (const std::string& line : cachedLines) {
                size_t colonPos = line.find(':');
                if (colonPos != std::string::npos) {
                    std::string label = line.substr(0, colonPos + 1);
                    std::string value = line.substr(colonPos + 1);

                    DrawTextEx(font, label.c_str(), { mousePos.x + padding, mousePos.y + padding + yOffset }, fontSize, 2, NookCol::TEXT_HIGHLIGHT);
                    int labelWidth = MeasureText(label.c_str(), fontSize);
                    DrawTextEx(font, value.c_str(), { mousePos.x + padding + labelWidth, mousePos.y + padding + yOffset }, fontSize, 2, NookCol::TEXT_DEFAULT);
                }
                else {
                    DrawTextEx(font, line.c_str(), { mousePos.x + padding, mousePos.y + padding + yOffset }, fontSize, 2, NookCol::TEXT_DEFAULT);
                }
                yOffset += fontSize + lineSpacing;
            }
        }
        else if (!lastHoveredNode) {
            cachedTooltipText.clear();
            hoverStartTime = 0.0;
        }



        EndDrawing();
    }


    UnloadFont(font);
    CloseWindow();

    return 0;
}