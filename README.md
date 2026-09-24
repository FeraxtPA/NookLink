# NookLink

NookLink is a C++23 desktop application built with raylib and libcurl. CMake is the build definition on Windows and Linux. The repository keeps the small nlohmann JSON header and tinyfiledialogs C source in `include/`; raylib and curl are compiled libraries managed through vcpkg on Windows.

## Windows: VS Code or Visual Studio 2022

Install the Visual Studio 2022 **Desktop development with C++** workload, CMake 3.25 or newer, vcpkg, and the VS Code **CMake Tools** and **C/C++** extensions. Copy `CMakeUserPresets.json.example` to `CMakeUserPresets.json` and set `VCPKG_ROOT` to your vcpkg directory. The local presets file is ignored by Git.

In VS Code, open the repository folder. Press **Ctrl+Shift+B** to configure and build Debug. In **Run and Debug**, choose **NookLink (CMake Debug)** and press **F5** to build and debug, or **Ctrl+F5** to run. If C/C++ Runner is installed, use its extension gear menu to **Disable (Workspace)**: it can regenerate GCC settings and a standalone-file launch profile that conflicts with this setup. CMake Tools supplies IntelliSense configuration.

In Visual Studio 2022, open the repository folder as a CMake project and select the `windows-local` configure preset. The old hand-maintained solution and project files are no longer part of the repository; CMake may generate a solution inside `build/`.

The same build is available from a terminal:

```powershell
cmake --preset windows-local
cmake --build --preset windows-debug --parallel 4
```

The executable is `build/cmake/windows/Debug/NookLink.exe`. Use `windows-release` for a Release build. CMake and vcpkg copy runtime DLLs and assets into the output directory. The launch configuration uses the repository root as the working directory for assets and the optional `.env` file.

## Linux

Install a C++23 compiler, CMake, Make, raylib development files with a `raylib.pc` pkg-config file, and libcurl development files. Then run:

```sh
cmake --preset linux-system
cmake --build --preset linux-debug -j4
ctest --preset linux-debug
./build/cmake/linux/NookLink
```

For Release, configure and build with `linux-release`. VS Code's build tasks select the platform's preset; use the **NookLink (Linux Debug)** launch profile with GDB on Linux.

The CMake build and tests have also been run on Linux. Add new application sources to `CMakeLists.txt`; it is the only maintained build definition. The asset directory is lowercase `assets/` on both platforms.

## Tests

Core logic is built in `nooklink_core` without a raylib window. The tests use disposable directories and never write to your library or application settings.

```powershell
cmake --build --preset windows-debug --parallel 4
ctest --preset windows-debug
```

Use the matching `windows-release`, `linux-debug`, or `linux-release` test preset after building it. VS Code also has a **Test NookLink (CMake Debug)** task. Coverage includes save/backup failure, Windows file locking, corrupt and missing primary files, invalid library schemas, ID lookup after sorting, next-ID repair, search parsing, date/rating validation, and settings paths.

To build just the core and tests on Linux without raylib or curl:

```sh
cmake -S . -B build/core-tests -DNOOKLINK_BUILD_APP=OFF -DCMAKE_BUILD_TYPE=Debug
cmake --build build/core-tests -j4
ctest --test-dir build/core-tests --output-on-failure
```

## Local settings and libraries

Windows stores settings at `%LOCALAPPDATA%/NookLink/settings.conf`. Linux uses `$XDG_CONFIG_HOME/nooklink/settings.conf`, defaulting to `~/.config/nooklink/settings.conf`. A new default library uses the Windows settings directory or `$XDG_DATA_HOME/nooklink/library.json` (default `~/.local/share/nooklink/library.json`) on Linux. Libraries selected with **Save As** stay at the chosen location.

On the first launch without modern settings, NookLink imports a legacy `.nooklink_config` from the working directory and remembers an existing `my_books.json`. The old files stay on disk. Personal settings, local libraries, backups, and generated executables are excluded from version control; earlier Git history is not rewritten.

Saving writes a complete temporary file beside the destination and replaces the original without deleting it first. An existing library gets a `.bak` copy; a backup failure aborts the save. Loading validates the whole library before changing live data. If the primary is damaged or missing, a valid `.bak` can be recovered, with a notification. Saving after recovery preserves that good backup. These safeguards do not provide a guarantee against power loss or simultaneous editing by multiple app instances.

## Cleanup checkpoint

The cleanup snapshot is stored locally under `build/cleanup-backup/`. It contains the source before cleanup and copies of the local settings and libraries, including the configured active library. Keep this directory while reviewing the changes; it is ignored by Git and contains personal data.

When checking application behavior interactively, load a library, edit and save it, restore it after reopening, save after deleting the final book, cancel **Save As**, and verify graph positions survive saving on exit.
