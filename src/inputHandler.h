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

	Node* m_LastClickedNode{ nullptr };
	float m_LastClickTime{ 0.0f };
	const float m_DoubleClickThreshold{ 0.3f };
};