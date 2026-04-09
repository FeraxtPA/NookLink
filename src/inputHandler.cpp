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

            // Double click logika z�st�v� stejn�...
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

                // === TADY JE TA ZM�NA ===
                if (clickedNode->type == NodeType::Book) {
                    // Zm�n�no z findBookById na getBookById (z�sk�me Book* m�sto const Book*)
                    Book* clickedBook = m_BookManager.getBookById(clickedNode->id);
                    if (clickedBook) {
                        Log::Debug("Clicked book: " + clickedBook->getTitle());

                        // ZAVOL�ME N�� NOV� PANEL!
                        m_UIManager->OpenBookDetails(clickedBook);
                    }
                }
                else if (clickedNode->type == NodeType::Genre) {
                    std::string genreName = m_GraphManager->getGenreNameByNodeId(clickedNode->id);
                    Log::Debug("Clicked genre: " + genreName);
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