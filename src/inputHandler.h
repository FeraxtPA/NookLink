
// Handles user input including mouse and keyboard interactions.
// Processes node selection, dragging, double-clicks, and keyboard shortcuts.
// Coordinates input events between graph visualization and UI system.

#pragma once
#include "raylib.h"
#include "graphManager.h"
#include "bookManager.h"
#include "uiManager.h"

class InputHandler
{
public:
	InputHandler(GraphManager* graphManager, BookManager& bookManager, UIManager* uiManager);

	void ProcessInputs(Vector2 worldMousePos, double currentTime, bool& layoutDirty, bool& hasUnsavedChanged);

private:
	void HandleMouseInteraction(Vector2 worldMousePos, double currentTime, bool& layoutDirty, bool& hasUnsavedChanged);
	void HandleKeyboardShortcuts(bool& layoutDirty);

	GraphManager* m_GraphManager;
	BookManager& m_BookManager;
	UIManager* m_UIManager;

	int m_LastClickedNodeId{ -1 };
	NodeType m_LastClickedNodeType{ NodeType::Book };
	float m_LastClickTime{ 0.0f };
	const float m_DoubleClickThreshold{ 0.3f };
};