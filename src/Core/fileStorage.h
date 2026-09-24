#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace FileStorage {
// Writes beside the destination, then replaces it without deleting the old file first.
bool WriteAtomically(const std::filesystem::path& target, std::string_view content,
                     std::string& error, bool keepBackup = true);
}
