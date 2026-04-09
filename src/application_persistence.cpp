
// Application data persistence and I/O.
// Handles configuration loading, saving, and file management.


#include "application.h"

#include "colors.h"
#include "logging.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

void Application::LoadConfig()
{
    std::ifstream configFile(".nooklink_config");
    if (configFile.is_open()) {
        std::string line;
        bool hasStructuredData = false;
        while (std::getline(configFile, line)) {
            if (line.empty()) continue;

            const size_t eqPos = line.find('=');
            // Backward compatibility: older config stored only a raw path per line.
            if (eqPos == std::string::npos) {
                if (!hasStructuredData && fs::exists(line)) {
                    m_SaveFileName = line;
                    Log::Info("Restored last session file: " + m_SaveFileName.string());
                }
                continue;
            }

            hasStructuredData = true;
            const std::string key = line.substr(0, eqPos);
            const std::string value = line.substr(eqPos + 1);

            if (key == "save_path") {
                if (fs::exists(value)) {
                    m_SaveFileName = value;
                    Log::Info("Restored last session file: " + m_SaveFileName.string());
                }
            }
            else if (key == "goal_target") {
                try {
                    m_ReadingGoalTarget = std::max(1, std::stoi(value));
                }
                catch (...) {}
            }
            else if (key == "goal_baseline") {
                try {
                    m_ReadingGoalBaselineRead = std::max(0, std::stoi(value));
                }
                catch (...) {}
            }
            else if (key == "theme_index") {
                try {
                    m_ThemePresetIndex = std::stoi(value);
                    NookCol::ApplyThemePresetByIndex(m_ThemePresetIndex);
                    m_ThemePresetIndex = NookCol::GetCurrentThemeIndex();
                }
                catch (...) {}
            }
            else if (key == "layout_density") {
                try {
                    m_LayoutDensityScale = std::clamp(std::stof(value), 0.3f, 1.6f);
                }
                catch (...) {}
            }
        }
        configFile.close();
    }
}

void Application::SaveConfig()
{
    // Persist only lightweight app state needed for restoring the next session.
    std::ofstream configFile(".nooklink_config");
    if (configFile.is_open()) {
        configFile << "save_path=" << m_SaveFileName.string() << "\n";
        configFile << "goal_target=" << m_ReadingGoalTarget << "\n";
        configFile << "goal_baseline=" << m_ReadingGoalBaselineRead << "\n";
        configFile << "theme_index=" << m_ThemePresetIndex << "\n";
        configFile << "layout_density=" << m_LayoutDensityScale << "\n";
        configFile.close();
    }
}

int Application::GetReadBooksCount() const
{
    int readCount = 0;
    for (const auto& book : m_BookManager.getBooks()) {
        if (book.getStatus() == Status::Read) {
            ++readCount;
        }
    }
    return readCount;
}

void Application::AdjustReadingGoalTarget(int delta)
{
    m_ReadingGoalTarget = std::clamp(m_ReadingGoalTarget + delta, 1, 10000);
    SaveConfig();
}

int Application::GetReadingGoalProgress() const
{
    return std::max(0, GetReadBooksCount() - m_ReadingGoalBaselineRead);
}

void Application::ResetReadingGoalProgressBaseline()
{
    m_ReadingGoalBaselineRead = GetReadBooksCount();
    SaveConfig();
}
