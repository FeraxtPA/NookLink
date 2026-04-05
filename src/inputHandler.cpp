#include "inputHandler.h"
#include <iostream>
#include <print>
#include <ranges>

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

            // Double click logika zùstává stejná...
            if (clickedNode == m_LastClickedNode && (currentTime - m_LastClickTime) <= m_DoubleClickThreshold) {
                if (m_GraphManager->tryUnlockNodeAt(worldMousePos)) {
                    std::cout << "Unlocked node ID: " << clickedNode->id << std::endl;
                    m_LastClickedNode = nullptr;
                    m_LastClickTime = 0.0;
                }
            }
            else {
                m_LastClickedNode = clickedNode;
                m_LastClickTime = currentTime;

                // === TADY JE TA ZMÌNA ===
                if (clickedNode->type == NodeType::Book) {
                    // Zmìnìno z findBookById na getBookById (získáme Book* místo const Book*)
                    Book* clickedBook = m_BookManager.getBookById(clickedNode->id);
                    if (clickedBook) {
                        std::cout << "Clicked book: " << clickedBook->getTitle() << std::endl;

                        // ZAVOLÁME NÁŠ NOVÝ PANEL!
                        m_UIManager->OpenBookDetails(clickedBook);
                    }
                }
                else if (clickedNode->type == NodeType::Genre) {
                    std::string genreName = m_GraphManager->getGenreNameByNodeId(clickedNode->id);
                    std::cout << "Clicked genre: " << genreName << std::endl;
                }
            }
        }
        else {
            m_LastClickedNode = nullptr;
        }
    }
}

void InputHandler::HandleKeyboardShortcuts(bool& layoutDirty) {
    
    if (IsKeyPressed(KEY_E)) {
        if (m_LastClickedNode && m_LastClickedNode->type == NodeType::Book) {
            Book* bookToEdit = m_BookManager.getBookById(m_LastClickedNode->id);
            if (bookToEdit) {
                m_UIManager->OpenEditPanel(bookToEdit);
            }
        }
    }

}