#define _CRT_SECURE_NO_WARNINGS
#include "appPaths.h"

#include <cstdlib>
#include <stdexcept>

namespace fs = std::filesystem;

namespace {
fs::path EnvironmentPath(const char* name)
{
    const char* value = std::getenv(name);
    if (!value || !*value) return {};
    const fs::path path(value);
    return path.is_absolute() ? path : fs::path{};
}

fs::path HomeDirectory()
{
#ifdef _WIN32
    const auto home = EnvironmentPath("USERPROFILE");
#else
    const auto home = EnvironmentPath("HOME");
#endif
    if (home.empty()) throw std::runtime_error("Cannot locate the user home directory");
    return home;
}
}

fs::path AppPaths::ConfigDirectory()
{
#ifdef _WIN32
    const auto local = EnvironmentPath("LOCALAPPDATA");
    return (local.empty() ? HomeDirectory() / "AppData" / "Local" : local) / "NookLink";
#else
    const auto config = EnvironmentPath("XDG_CONFIG_HOME");
    return (config.empty() ? HomeDirectory() / ".config" : config) / "nooklink";
#endif
}

fs::path AppPaths::DataDirectory()
{
#ifdef _WIN32
    return ConfigDirectory();
#else
    const auto data = EnvironmentPath("XDG_DATA_HOME");
    return (data.empty() ? HomeDirectory() / ".local" / "share" : data) / "nooklink";
#endif
}

fs::path AppPaths::ConfigFile() { return ConfigDirectory() / "settings.conf"; }
fs::path AppPaths::DefaultLibraryFile() { return DataDirectory() / "library.json"; }
