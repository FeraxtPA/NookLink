#include "logging.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

namespace fs = std::filesystem;

namespace {

std::mutex g_LogMutex;
std::ofstream g_LogFile;
bool g_Initialized = false;

const char* levelToString(Log::Level level) {
    switch (level) {
    case Log::Level::Debug: return "DEBUG";
    case Log::Level::Info: return "INFO";
    case Log::Level::Warn: return "WARN";
    case Log::Level::Error: return "ERROR";
    }
    return "UNKNOWN";
}

fs::path getLogDirectory() {
#ifdef _WIN32
    const char* appData = std::getenv("APPDATA");
    if (appData != nullptr) {
        return fs::path(appData) / "NookLink" / "logs";
    }
#endif

    const char* home = std::getenv("HOME");
    if (home != nullptr) {
        return fs::path(home) / ".nooklink" / "logs";
    }

    return fs::current_path() / "logs";
}

std::string nowForLine() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t timeNow = std::chrono::system_clock::to_time_t(now);

    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &timeNow);
#else
    localtime_r(&timeNow, &tm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string todayFileName() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t timeNow = std::chrono::system_clock::to_time_t(now);

    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &timeNow);
#else
    localtime_r(&timeNow, &tm);
#endif

    std::ostringstream oss;
    oss << "nooklink-" << std::put_time(&tm, "%Y-%m-%d") << ".log";
    return oss.str();
}

void ensureInitializedLocked() {
    if (g_Initialized) {
        return;
    }

    std::error_code ec;
    const fs::path logDir = getLogDirectory();
    fs::create_directories(logDir, ec);

    const fs::path logPath = logDir / todayFileName();
    g_LogFile.open(logPath, std::ios::app);
    g_Initialized = true;

    if (!g_LogFile.is_open()) {
        std::cerr << "[WARN] Failed to open log file: " << logPath << std::endl;
    }
}

} // namespace

namespace Log {

void Init() {
    std::lock_guard<std::mutex> lock(g_LogMutex);
    ensureInitializedLocked();
}

void Shutdown() {
    std::lock_guard<std::mutex> lock(g_LogMutex);
    if (g_LogFile.is_open()) {
        g_LogFile.flush();
        g_LogFile.close();
    }
    g_Initialized = false;
}

void Write(Level level, const std::string& message) {
    std::lock_guard<std::mutex> lock(g_LogMutex);
    ensureInitializedLocked();

    const std::string line = nowForLine() + " [" + levelToString(level) + "] " + message;

    if (g_LogFile.is_open()) {
        g_LogFile << line << '\n';
        g_LogFile.flush();
    }

    if (level == Level::Error || level == Level::Warn) {
        std::cerr << line << std::endl;
    }
    else {
        std::cout << line << std::endl;
    }
}

void Debug(const std::string& message) { Write(Level::Debug, message); }
void Info(const std::string& message) { Write(Level::Info, message); }
void Warn(const std::string& message) { Write(Level::Warn, message); }
void Error(const std::string& message) { Write(Level::Error, message); }

} // namespace Log
