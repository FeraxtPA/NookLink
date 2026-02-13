#pragma once
#include "raylib.h"
#include "bookManager.h"
#include "graphManager.h"

class DebugManager
{
public:
	DebugManager(BookManager& bookManager, GraphManager* graphManager);

	void HandleDebugInputs(bool& layoutDirty);

private:
	BookManager& m_BookManager;
	GraphManager* m_GraphManager;

};