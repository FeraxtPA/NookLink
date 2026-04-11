
// Handles user input including mouse and keyboard interactions.
// Processes node selection, dragging, double-clicks, and keyboard shortcuts.
// Coordinates input events between graph visualization and UI system.

#pragma once
#include "raylib.h"
#include "nodeRenderer.h"

class GraphManager;
class BookManager;
class UIManager;

class InputHandler
{
public:
	InputHandler(GraphManager* graphManager, BookManager& bookManager, UIManager* uiManager);

  void ProcessInputs(Vector2 worldMousePos, double currentTime, bool& layoutDirty);
  bool IsMultiSelectMode() const { return m_MultiSelectMode; }

private:
 void HandleMouseInteraction(Vector2 worldMousePos, double currentTime, bool& layoutDirty);
	void HandleKeyboardShortcuts();

	GraphManager* m_GraphManager;
	BookManager& m_BookManager;
	UIManager* m_UIManager;

	int m_LastClickedNodeId{ -1 };
	NodeType m_LastClickedNodeType{ NodeType::Book };
	float m_LastClickTime{ 0.0f };
	const float m_DoubleClickThreshold{ 0.3f };

	bool m_MultiSelectMode = false;
	bool m_IsSelectionDragging = false;
    bool m_IsMultiDragging = false;
	Vector2 m_SelectionStartWorld{ 0.0f, 0.0f };
	Vector2 m_SelectionCurrentWorld{ 0.0f, 0.0f };
  Vector2 m_LastMultiDragWorld{ 0.0f, 0.0f };
};