#pragma once

#include <filesystem>

namespace AppPaths {
std::filesystem::path ConfigDirectory();
std::filesystem::path DataDirectory();
std::filesystem::path ConfigFile();
std::filesystem::path DefaultLibraryFile();
}
