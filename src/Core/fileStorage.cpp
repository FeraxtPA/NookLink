#include "fileStorage.h"

#include <fstream>
#include <random>
#include <system_error>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace {
void ReplaceFile(const fs::path& source, const fs::path& target)
{
#ifdef _WIN32
    if (!MoveFileExW(source.c_str(), target.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        throw std::system_error(static_cast<int>(GetLastError()), std::system_category(),
                                "Could not replace " + target.string());
    }
#else
    fs::rename(source, target);
#endif
}

struct StagingDirectory {
    fs::path path;
    ~StagingDirectory() {
        std::error_code ignored;
        // path is an exclusively created directory owned by this save operation.
        if (!path.empty()) fs::remove_all(path, ignored);
    }
};
}

bool FileStorage::WriteAtomically(const fs::path& target, std::string_view content,
                                std::string& error, bool keepBackup)
{
    error.clear();
    try {
        if (target.empty()) throw std::runtime_error("The target filename is empty");
        const bool exists = fs::exists(target);
        if (fs::is_symlink(target) || (exists && !fs::is_regular_file(target))) {
            throw std::runtime_error("The target must be a regular file: " + target.string());
        }

        StagingDirectory staging;
        std::random_device random;
        for (int attempt = 0; attempt < 16; ++attempt) {
            fs::path candidate = target;
            candidate += ".save-" + std::to_string(random());
            if (fs::create_directory(candidate)) {
                staging.path = std::move(candidate);
                break;
            }
        }
        if (staging.path.empty()) throw std::runtime_error("Could not create a save staging directory");

        const fs::path pending = staging.path / "content";
        std::ofstream output(pending, std::ios::binary | std::ios::trunc);
        output.write(content.data(), static_cast<std::streamsize>(content.size()));
        output.flush();
        output.close();
        if (!output) throw std::runtime_error("Could not write the complete file: " + target.string());

        if (keepBackup && exists) {
            fs::path backup = target;
            backup += ".bak";
            if (fs::is_symlink(backup) || (fs::exists(backup) && !fs::is_regular_file(backup))) {
                throw std::runtime_error("The backup path is not a regular file: " + backup.string());
            }
            const fs::path pendingBackup = staging.path / "backup";
            fs::copy_file(target, pendingBackup);
            ReplaceFile(pendingBackup, backup);
        }

        ReplaceFile(pending, target);
        return true;
    }
    catch (const std::exception& exception) {
        error = exception.what();
        return false;
    }
}
