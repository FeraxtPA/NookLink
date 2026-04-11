
// Handles debug input commands and visual/logging utilities.
// Provides debug-only features for development and troubleshooting.


#pragma once

class BookManager;

class DebugManager
{
public:
 DebugManager(BookManager& bookManager);

  void HandleDebugInputs();

private:
	BookManager& m_BookManager;

};