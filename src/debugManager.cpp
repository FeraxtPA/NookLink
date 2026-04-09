
// Implementation of the DebugManager class.
// Provides debug input handling and testing utilities for development.


#include "debugManager.h"
#include "logging.h"
#include <print>
#include <ranges>

DebugManager::DebugManager(BookManager& bookManager, GraphManager* graphManager)
    : m_BookManager(bookManager), m_GraphManager(graphManager) {}

void DebugManager::HandleDebugInputs(bool& layoutDirty) {
    
    if (IsKeyPressed(KEY_M)) {
        for (int i = 0; i < 50; i++) {
            Book newBook("Test Book " + std::to_string(i), "Test Author", Status::ToRead);
            newBook.addGenre("History");
            m_BookManager.addBook(newBook);
        }
        m_GraphManager->initializePositions();
        layoutDirty = true;
        Log::Debug("Spawned 50 test books");
    }

  
    if (IsKeyPressed(KEY_V)) {
        SetTargetFPS(0);
        Log::Debug("FPS un-capped");
    }
    if (IsKeyPressed(KEY_B)) {
        SetTargetFPS(GetMonitorRefreshRate(0));
        Log::Debug("FPS capped to monitor refresh rate");
    }

   
    if (IsKeyPressed(KEY_T)) {
        std::print("[DEBUG] Books with 'To Read' status:\n");
        for (const auto& [i, book] : m_BookManager.getBooksToBeRead() | std::views::enumerate) {
            std::print("{} {}-{}\n", i + 1, book.getTitle(), book.getAuthor());
        }
    }
}

