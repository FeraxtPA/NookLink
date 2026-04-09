#pragma once

#include <string>

namespace Log {

enum class Level {
    Debug,
    Info,
    Warn,
    Error
};

void Init();
void Shutdown();

void Write(Level level, const std::string& message);

void Debug(const std::string& message);
void Info(const std::string& message);
void Warn(const std::string& message);
void Error(const std::string& message);

} // namespace Log
