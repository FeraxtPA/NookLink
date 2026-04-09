
// Implementation of the InputHandler class.
// Processes mouse interactions and keyboard shortcuts for the graph interface.


#include "inputHandler.h"
#include "logging.h"

InputHandler::InputHandler(GraphManager* graphManager, BookManager& bookManager, UIManager* uiManager)
    : m_GraphManager(graphManager), m_BookManager(bookManager), m_UIManager(uiManager) {}

void InputHandler::ProcessInputs(Vector2 worldMousePos, double currentTime, bool& layoutDirty, bool& hasUnsavedChanged) {
    HandleMouseInteraction(worldMousePos, currentTime, layoutDirty,hasUnsavedChanged);
    HandleKeyboardShortcuts(layoutDirty);
}

void InputHandler::HandleMouseInteraction(Vector2 worldMousePos, double currentTime, bool& layoutDirty, bool& hasUnsavedChanged) {
    
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        if (m_GraphManager->TryGrabNodeAt(worldMousePos, IsKeyDown(KEY_LEFT_SHIFT))) {
            layoutDirty = true;
        }
    }

    if (IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) {
        m_GraphManager->releaseDraggedNode();
    }

    if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
        if (m_GraphManager->updateDraggedNodePosition(worldMousePos)) {
            layoutDirty = true;
        }
    }

   
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Node* clickedNode = m_GraphManager->getNodeAtPosition(worldMousePos);
        if (clickedNode) {

            if (clickedNode->id == m_LastClickedNodeId && (currentTime - m_LastClickTime) <= m_DoubleClickThreshold) {
                if (m_GraphManager->tryUnlockNodeAt(worldMousePos)) {
                    Log::Debug("Unlocked node ID: " + std::to_string(clickedNode->id));
                    m_LastClickedNodeId = -1;
                    m_LastClickTime = 0.0;
                }
            }
            else {
                m_LastClickedNodeId = clickedNode->id;
                m_LastClickedNodeType = clickedNode->type;
                m_LastClickTime = currentTime;

                if (clickedNode->type == NodeType::Book) {
                    Book* clickedBook = m_BookManager.getBookById(clickedNode->id);
                    if (clickedBook) {
                        Log::Debug("Clicked book: " + clickedBook->getTitle());

                        m_UIManager->OpenBookDetails(clickedBook);
                    }
                }
                else if (clickedNode->type == NodeType::Genre) {
                    std::string genreName = m_GraphManager->getGenreNameByNodeId(clickedNode->id);
                    Log::Debug("Clicked genre: " + genreName);

                    std::vector<std::string> sampleTitles;
                    for (const auto& b : m_BookManager.getBooks()) {
                        for (const auto& g : b.getGenres()) {
                            if (g == genreName) {
                                sampleTitles.push_back(b.getTitle());
                                break;
                            }
                        }
                    }

                    m_UIManager->OpenGenreDetails(
                        genreName,
                        m_GraphManager->getNumOfConnectedBooks(clickedNode->id),
                        sampleTitles
                    );
                }
            }
        }
        else {
            m_LastClickedNodeId = -1;
        }
    }
}

void InputHandler::HandleKeyboardShortcuts(bool& layoutDirty) {
    
    if (IsKeyPressed(KEY_E)) {
        if (m_LastClickedNodeId != -1 && m_LastClickedNodeType == NodeType::Book) {
            Book* bookToEdit = m_BookManager.getBookById(m_LastClickedNodeId);
            if (bookToEdit) {
                m_UIManager->OpenEditPanel(bookToEdit);
            }
        }
    }

}