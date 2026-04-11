
// Implementation of the InputHandler class.
// Processes mouse interactions and keyboard shortcuts for the graph interface.


#include "inputHandler.h"
#include "bookManager.h"
#include "graphManager.h"
#include "logging.h"
#include "uiManager.h"



InputHandler::InputHandler(GraphManager* graphManager, BookManager& bookManager, UIManager* uiManager)
    : m_GraphManager(graphManager), m_BookManager(bookManager), m_UIManager(uiManager) {}

void InputHandler::ProcessInputs(Vector2 worldMousePos, double currentTime, bool& layoutDirty) {
    HandleMouseInteraction(worldMousePos, currentTime, layoutDirty);
    HandleKeyboardShortcuts();
}

void InputHandler::HandleMouseInteraction(Vector2 worldMousePos, double currentTime, bool& layoutDirty) {

    if (m_MultiSelectMode) {
        const bool shiftDown = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

        if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
            Node* clicked = m_GraphManager->getNodeAtPosition(worldMousePos);
            if (clicked && clicked->type == NodeType::Book && m_GraphManager->IsBookMultiSelected(clicked->id)) {
                m_IsMultiDragging = true;
                m_LastMultiDragWorld = worldMousePos;
            }
        }

        if (m_IsMultiDragging && IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
            const Vector2 delta = Vector2Subtract(worldMousePos, m_LastMultiDragWorld);
            if (std::abs(delta.x) > 0.001f || std::abs(delta.y) > 0.001f) {
                if (m_GraphManager->TranslateMultiSelectedBookNodes(delta)) {
                    m_LastMultiDragWorld = worldMousePos;
                }
            }
        }

        if (m_IsMultiDragging && IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) {
            m_IsMultiDragging = false;
        }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            m_IsSelectionDragging = true;
            m_SelectionStartWorld = worldMousePos;
            m_SelectionCurrentWorld = worldMousePos;
            m_GraphManager->SetSelectionRectangleWorld(m_SelectionStartWorld, m_SelectionCurrentWorld);
        }

        if (m_IsSelectionDragging && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            m_SelectionCurrentWorld = worldMousePos;
            m_GraphManager->SetSelectionRectangleWorld(m_SelectionStartWorld, m_SelectionCurrentWorld);
        }

        if (m_IsSelectionDragging && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            m_IsSelectionDragging = false;

            const float dx = m_SelectionCurrentWorld.x - m_SelectionStartWorld.x;
            const float dy = m_SelectionCurrentWorld.y - m_SelectionStartWorld.y;
            const bool isClickLike = (std::abs(dx) < 8.0f && std::abs(dy) < 8.0f);

            std::vector<int> selectedIds;
            if (isClickLike) {
                Node* clicked = m_GraphManager->getNodeAtPosition(worldMousePos);
                if (clicked && clicked->type == NodeType::Book) {
                    selectedIds.push_back(clicked->id);
                }
            }
            else {
                Rectangle rect{ m_SelectionStartWorld.x, m_SelectionStartWorld.y, dx, dy };
                selectedIds = m_GraphManager->GetBookNodeIdsInWorldRect(rect, true);
            }

            if (shiftDown) {
                m_GraphManager->AddMultiSelectedBookIds(selectedIds);
            }
            else {
                m_GraphManager->SetMultiSelectedBookIds(selectedIds);
            }
            m_GraphManager->ClearSelectionRectangleWorld();

            if (m_UIManager) {
                m_UIManager->ShowNotification("Selected books total: " + std::to_string(m_GraphManager->GetMultiSelectedBookCount()) + ". Press G to assign genre.");
                m_UIManager->SetMultiSelectIndicator(true, (int)m_GraphManager->GetMultiSelectedBookCount());
            }
        }

        return;
    }
    
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        if (m_GraphManager->TryGrabNodeAt(worldMousePos, IsKeyDown(KEY_LEFT_SHIFT))) {
            m_GraphManager->setPhysicsActive(false);
            layoutDirty = false;
        }
    }

    if (IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) {
        m_GraphManager->releaseDraggedNode();
    }

    if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
        if (m_GraphManager->updateDraggedNodePosition(worldMousePos)) {
            layoutDirty = false;
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
                m_LastClickTime = static_cast<float>(currentTime);

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

void InputHandler::HandleKeyboardShortcuts() {
    if (m_UIManager && m_UIManager->IsSearchBarFocused()) {
        return;
    }

    if (m_MultiSelectMode && IsKeyPressed(KEY_ESCAPE)) {
        m_IsSelectionDragging = false;
        if (m_GraphManager) {
            m_GraphManager->ClearSelectionRectangleWorld();
            m_GraphManager->ClearMultiSelectedBookIds();
            m_GraphManager->releaseDraggedNode();
        }
        m_MultiSelectMode = false;
        if (m_UIManager) {
            m_UIManager->ShowNotification("Multi-select cleared (Esc).");
        }
        return;
    }

    if (IsKeyPressed(KEY_TAB)) {
        m_MultiSelectMode = !m_MultiSelectMode;

        if (!m_MultiSelectMode) {
            m_IsSelectionDragging = false;
            m_IsMultiDragging = false;
            if (m_GraphManager) {
                m_GraphManager->ClearSelectionRectangleWorld();
                m_GraphManager->ClearMultiSelectedBookIds();
            }
        }

        if (m_UIManager) {
            m_UIManager->ShowNotification(m_MultiSelectMode
                ? "Multi-select mode: ON (drag rectangle, G = assign genre)"
                : "Multi-select mode: OFF");
            m_UIManager->SetMultiSelectIndicator(m_MultiSelectMode, m_MultiSelectMode && m_GraphManager ? (int)m_GraphManager->GetMultiSelectedBookCount() : 0);
        }
        return;
    }

    if (m_MultiSelectMode && IsKeyPressed(KEY_G)) {
        if (!m_GraphManager || m_GraphManager->GetMultiSelectedBookCount() == 0) {
            if (m_UIManager) m_UIManager->ShowNotification("No selected books. Drag rectangle first.");
            return;
        }
        if (m_UIManager) {
            m_UIManager->OpenBulkGenreAssignPanel((int)m_GraphManager->GetMultiSelectedBookCount());
        }
        return;
    }

    
    if (IsKeyPressed(KEY_E)) {
        if (m_LastClickedNodeId != -1 && m_LastClickedNodeType == NodeType::Book) {
            Book* bookToEdit = m_BookManager.getBookById(m_LastClickedNodeId);
            if (bookToEdit) {
                m_UIManager->OpenEditPanel(bookToEdit);
            }
        }
    }

}