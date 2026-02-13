#include "inputHandler.h"
#include <iostream>
#include <print>
#include <ranges>

InputHandler::InputHandler(GraphManager* graphManager, BookManager& bookManager, UIManager* uiManager)
    : m_GraphManager(graphManager), m_BookManager(bookManager), m_UIManager(uiManager) {}

void InputHandler::ProcessInputs(Vector2 worldMousePos, double currentTime, bool& layoutDirty) {
    HandleMouseInteraction(worldMousePos, currentTime, layoutDirty);
    HandleKeyboardShortcuts(layoutDirty);
}

void InputHandler::HandleMouseInteraction(Vector2 worldMousePos, double currentTime, bool& layoutDirty) {
    
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
         
            if (IsKeyDown(KEY_LEFT_SHIFT) && clickedNode->type == NodeType::Book) {
                std::cout << "Removing book node with ID: " << clickedNode->id << std::endl;
                m_BookManager.removeBook(clickedNode->id);
                m_GraphManager->removeNodeById(clickedNode->id);
                layoutDirty = true;
                m_LastClickedNode = nullptr;
                m_LastClickTime = 0.0;
            }
           
            else if (clickedNode == m_LastClickedNode && (currentTime - m_LastClickTime) <= m_DoubleClickThreshold) {
                if (m_GraphManager->tryUnlockNodeAt(worldMousePos)) {
                    std::cout << "Unlocked node ID: " << clickedNode->id << std::endl;
                    m_LastClickedNode = nullptr;
                    m_LastClickTime = 0.0;
                }
            }
           
            else {
                m_LastClickedNode = clickedNode;
                m_LastClickTime = currentTime;

                if (clickedNode->type == NodeType::Book) {
                    const Book* clickedBook = m_BookManager.findBookById(clickedNode->id);
                    if (clickedBook) std::cout << "Clicked book: " << clickedBook->getTitle() << std::endl;
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