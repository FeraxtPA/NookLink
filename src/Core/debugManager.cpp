
// Implementation of the DebugManager class.
// Provides debug input handling and testing utilities for development.


#include "debugManager.h"
#include "bookManager.h"
#include "logging.h"
#include "raylib.h"
#include <print>
#include <ranges>

DebugManager::DebugManager(BookManager& bookManager)
    : m_BookManager(bookManager) {}

void DebugManager::HandleDebugInputs() {
    
    

  
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

