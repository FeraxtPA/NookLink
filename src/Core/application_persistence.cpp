
// Application data persistence and I/O.
// Handles configuration loading, saving, and file management.


#include "application.h"

#include "colors.h"
#include "logging.h"
#include "appPaths.h"
#include "fileStorage.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cmath>

namespace fs = std::filesystem;

void Application::LoadConfig()
{
    fs::path configPath;
    bool migrateLegacy = false;
    try {
        m_SaveFileName = AppPaths::DefaultLibraryFile();
        configPath = AppPaths::ConfigFile();
        if (!fs::exists(configPath)) {
            migrateLegacy = fs::exists(".nooklink_config") || fs::exists("my_books.json");
            configPath = ".nooklink_config";
            if (fs::exists("my_books.json")) m_SaveFileName = fs::absolute("my_books.json");
        }
    }
    catch (const std::exception& error) {
        Log::Warn("Could not locate settings: " + std::string(error.what()));
        return;
    }
    const auto hasLibrary = [](const fs::path& path) {
        std::error_code error;
        if (fs::is_regular_file(path, error)) return true;
        fs::path backup = path;
        backup += ".bak";
        return fs::is_regular_file(backup, error);
    };
    std::ifstream configFile(configPath);
    if (configFile.is_open()) {
        std::string line;
        bool hasStructuredData = false;
        bool firstLine = true;
        while (std::getline(configFile, line)) {
            if (firstLine && line.starts_with("\xEF\xBB\xBF")) line.erase(0, 3);
            firstLine = false;
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) continue;

            const size_t eqPos = line.find('=');
            // Backward compatibility: older config stored only a raw path per line.
            if (eqPos == std::string::npos) {
                if (!hasStructuredData && hasLibrary(line)) {
                    m_SaveFileName = fs::absolute(line);
                    Log::Info("Restored last session file: " + m_SaveFileName.string());
                }
                continue;
            }

            hasStructuredData = true;
            const std::string key = line.substr(0, eqPos);
            const std::string value = line.substr(eqPos + 1);

            if (key == "save_path") {
                if (hasLibrary(value)) {
                    m_SaveFileName = fs::absolute(value);
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
                    const float density = std::stof(value);
                    if (std::isfinite(density)) m_LayoutDensityScale = std::clamp(density, 0.3f, 1.6f);
                }
                catch (...) {}
            }
        }
        configFile.close();
    }
    if (migrateLegacy) SaveConfig();
}

void Application::SaveConfig()
{
    try {
        const auto configPath = AppPaths::ConfigFile();
        fs::create_directories(configPath.parent_path());
        std::ostringstream configFile;
        configFile << "save_path=" << m_SaveFileName.string() << "\n";
        configFile << "goal_target=" << m_ReadingGoalTarget << "\n";
        configFile << "goal_baseline=" << m_ReadingGoalBaselineRead << "\n";
        configFile << "theme_index=" << m_ThemePresetIndex << "\n";
        configFile << "layout_density=" << m_LayoutDensityScale << "\n";
        std::string error;
        if (!FileStorage::WriteAtomically(configPath, configFile.str(), error)) {
            Log::Warn("Could not save settings: " + error);
        }
    }
    catch (const std::exception& error) {
        Log::Warn("Could not save settings: " + std::string(error.what()));
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
