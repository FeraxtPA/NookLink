#include "appPaths.h"
#include "bookManager.h"
#include "date_utils.h"
#include "fileStorage.h"
#include "logging.h"
#include "searchFilter.h"
#include "validation.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace fs = std::filesystem;
using Positions = std::unordered_map<int, NodePosition>;

#define CHECK(condition) do { if (!(condition)) throw std::runtime_error( \
    std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " #condition); } while (false)

namespace {
void SetEnvironment(const char* name, const fs::path& path)
{
#ifdef _WIN32
    CHECK(_putenv_s(name, path.string().c_str()) == 0);
#else
    CHECK(setenv(name, path.string().c_str(), 1) == 0);
#endif
}

struct TemporaryDirectory {
    fs::path path;
    TemporaryDirectory() {
        std::random_device random;
        for (int attempt = 0; attempt < 16; ++attempt) {
            auto candidate = fs::temp_directory_path() / ("nooklink-tests-" + std::to_string(random()));
            if (fs::create_directory(candidate)) {
                path = std::move(candidate);
                return;
            }
        }
        throw std::runtime_error("Cannot create test directory");
    }
    ~TemporaryDirectory() {
        Log::Shutdown();
        std::error_code ignored;
        fs::remove_all(path, ignored);
    }
};

void Write(const fs::path& path, const std::string& content)
{
    std::ofstream output(path, std::ios::binary);
    output << content;
    output.close();
    CHECK(output.good());
}

std::string Read(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    CHECK(input.good());
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

Book ExampleBook(std::string title = "A book", std::string author = "An author")
{
    Book book(title, author, Status::Read);
    book.setRating(4.25f);
    book.addGenre("Fantasy");
    book.setPageCount(321);
    book.setNotes("Quotes: \"hello\"\nPříliš žluťoučký kůň");
    book.setDateFinishedReading("29.02.2024");
    return book;
}

void Roundtrip(const fs::path& root)
{
    BookManager original;
    const int id = original.addBook(ExampleBook());
    Positions positions{{id, {12.5f, -9.0f, true}}, {-1, {3.0f, 4.0f, false}}};
    const auto path = root / "library.json";
    CHECK(original.saveBooksToFile(path.string(), positions));
    BookManager loaded;
    Positions restored;
    CHECK(loaded.loadBooksFromFile(path.string(), restored));
    CHECK(nlohmann::json(original.getBooks()) == nlohmann::json(loaded.getBooks()));
    CHECK(restored.size() == 2);
    CHECK(restored.at(id).x == 12.5f && restored.at(id).y == -9.0f && restored.at(id).locked);
    CHECK(restored.at(-1).x == 3.0f);
    CHECK(!loaded.wasRecoveredFromBackup());

    loaded.removeBook(id);
    CHECK(loaded.saveBooksToFile(path.string()));
    CHECK(original.loadBooksFromFile(path.string(), restored));
    CHECK(original.getBooks().empty() && restored.empty());
}

void Backup(const fs::path& root)
{
    BookManager manager;
    const int id = manager.addBook(ExampleBook("First version"));
    const auto path = root / "library.json";
    CHECK(manager.saveBooksToFile(path.string()));
    const auto first = Read(path);
    manager.getBookById(id)->setTitle("Second version");
    CHECK(manager.saveBooksToFile(path.string()));
    CHECK(Read(root / "library.json.bak") == first);
    CHECK(nlohmann::json::parse(Read(path))["books"][0]["title"] == "Second version");
}

void SaveFailure(const fs::path& root)
{
    BookManager manager;
    manager.addBook(ExampleBook());
    const auto path = root / "library.json";
    CHECK(manager.saveBooksToFile(path.string()));
    const auto original = Read(path);
    // An obstructed backup path must abort saving before the primary is replaced.
    fs::create_directory(root / "library.json.bak");
    Write(root / "library.json.bak" / "keep", "preserve me");
    CHECK(!manager.saveBooksToFile(path.string()));
    CHECK(Read(path) == original);
    CHECK(Read(root / "library.json.bak" / "keep") == "preserve me");
    CHECK(!manager.getLastError().empty());
    CHECK(!manager.saveBooksToFile((root / "missing" / "library.json").string()));
    CHECK(!manager.saveBooksToFile(""));
    CHECK(!manager.saveBooksToFile(root.string()));
    for (const auto& entry : fs::directory_iterator(root)) {
        CHECK(entry.path().filename().string().find(".save-") == std::string::npos);
    }
}

void LockedTarget(const fs::path& root)
{
#ifdef _WIN32
    BookManager manager;
    const int id = manager.addBook(ExampleBook());
    const auto path = root / "library.json";
    CHECK(manager.saveBooksToFile(path.string()));
    const auto original = Read(path);
    struct LockedFile {
        HANDLE handle;
        ~LockedFile() { if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle); }
    } locked{CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)};
    CHECK(locked.handle != INVALID_HANDLE_VALUE);
    manager.getBookById(id)->setTitle("Must not replace locked file");
    CHECK(!manager.saveBooksToFile(path.string()));
    CHECK(Read(path) == original);
    CHECK(Read(root / "library.json.bak") == original);
#else
    (void)root;
#endif
}

void Recovery(const fs::path& root)
{
    BookManager manager;
    manager.addBook(ExampleBook());
    const auto path = root / "library.json";
    CHECK(manager.saveBooksToFile(path.string()));
    const auto good = Read(path);
    Write(root / "library.json.bak", good);
    Write(path, "{ damaged");
    Positions positions;
    CHECK(manager.loadBooksFromFile(path.string(), positions));
    CHECK(manager.wasRecoveredFromBackup());
    CHECK(manager.getLastError().empty());
    CHECK(manager.saveBooksToFile(path.string()));
    CHECK(Read(root / "library.json.bak") == good);
    CHECK(!manager.wasRecoveredFromBackup());

    fs::remove(path);
    CHECK(manager.loadBooksFromFile(path.string(), positions));
    CHECK(manager.wasRecoveredFromBackup());
    CHECK(manager.saveBooksToFile(path.string()));
    CHECK(fs::is_regular_file(path));

    Write(path, "broken");
    Write(root / "library.json.bak", "also broken");
    positions = {{42, {1, 2, true}}};
    const auto before = nlohmann::json(manager.getBooks());
    CHECK(!manager.loadBooksFromFile(path.string(), positions));
    CHECK(nlohmann::json(manager.getBooks()) == before);
    CHECK(positions.size() == 1 && positions.at(42).locked);
}

void InvalidLibrary(const fs::path& root)
{
    BookManager manager;
    manager.addBook(ExampleBook("Keep this library"));
    const auto before = nlohmann::json(manager.getBooks());
    const auto path = root / "invalid.json";
    const std::vector<std::string> invalid = {
        "{}", "[]", R"({"books":null})", R"({"books":[{}]})",
        R"({"books":[{"id":1},{"id":1}]})",
        R"({"books":[{"id":-1}]})", R"({"books":[{"id":2147483647}]})",
        R"({"books":[{"id":4294967297}]})",
        R"({"books":[{"id":1,"rating":12}]})",
        R"({"books":[],"next_id":0})", R"({"books":[],"format_version":2})",
        R"({"books":[],"positions":{"1junk":{"x":0,"y":0}}})",
        R"({"books":[],"positions":{"1":{"x":null,"y":0}}})"
    };
    for (const auto& document : invalid) {
        Write(path, document);
        Positions positions{{99, {8, 9, true}}};
        CHECK(!manager.loadBooksFromFile(path.string(), positions));
        CHECK(nlohmann::json(manager.getBooks()) == before);
        CHECK(positions.size() == 1 && positions.at(99).x == 8);
    }
}

void NextId(const fs::path& root)
{
    const auto path = root / "old-library.json";
    Write(path, R"({"books":[{"id":8,"title":"Existing"}],"next_id":1})");
    BookManager manager;
    Positions positions;
    CHECK(manager.loadBooksFromFile(path.string(), positions));
    CHECK(manager.addBook(ExampleBook()) == 9);
    CHECK(manager.findBookById(8)->getTitle() == "Existing");
    Write(path, R"({"books":[{"id":2147483646}],"next_id":2147483647})");
    CHECK(manager.loadBooksFromFile(path.string(), positions));
    bool threw = false;
    try { manager.addBook(ExampleBook()); } catch (const std::overflow_error&) { threw = true; }
    CHECK(threw && manager.getBooks().size() == 1);
}

void Lookup(const fs::path&)
{
    BookManager manager;
    Book first = ExampleBook("First", "Zebra");
    first.setRating(1);
    first.setPageCount(10);
    first.setDateAdded("01.01.2020");
    Book second = ExampleBook("Second", "Alpha");
    second.setDateAdded("01.01.2024");
    const int firstId = manager.addBook(first);
    const int secondId = manager.addBook(second);
    for (auto mode : {BookSortMode::AuthorAsc, BookSortMode::RatingDesc,
                      BookSortMode::DateAddedDesc, BookSortMode::PageCountDesc, BookSortMode::IdAsc}) {
        manager.sortBooks(mode);
        CHECK(manager.getBookById(firstId)->getTitle() == "First");
        CHECK(manager.findBookById(secondId)->getTitle() == "Second");
    }
    manager.sortBooks(BookSortMode::AuthorAsc);
    const Book deleted = *manager.findBookById(firstId);
    manager.removeBook(firstId);
    CHECK(manager.findBookById(firstId) == nullptr);
    CHECK(manager.getBookById(secondId)->getTitle() == "Second");
    CHECK(manager.restoreBook(deleted));
    CHECK(!manager.restoreBook(deleted));
    CHECK(manager.findBookById(firstId)->getTitle() == "First");
    const auto newId = manager.addBook(ExampleBook("Third"));
    CHECK(newId > secondId && manager.findBookById(newId)->getTitle() == "Third");
    auto copy = manager;
    copy.removeBook(firstId);
    CHECK(manager.findBookById(firstId) != nullptr && copy.findBookById(firstId) == nullptr);
}

void Search(const fs::path&)
{
    Book book = ExampleBook("Doctor: The Journey", "Writer");
    SearchFilter filter;
    filter.setQuery("  doctor: the journey  ");
    CHECK(filter.matchesBook(&book));
    filter.setQuery("\tdoctor | r>4 | g:fantasy | s:read\t");
    CHECK(filter.matchesBook(&book));
    book.setStatus(Status::ToRead);
    CHECK(!filter.matchesBook(&book));
    filter.setQuery("s:toread");
    CHECK(filter.matchesBook(&book));
    for (const auto& query : {"r>", "r>garbage", "r>4junk", "r:nan", "r<inf", "r=-1", "r=6"}) {
        filter.setQuery(query);
        CHECK(!filter.matchesBook(&book));
    }
    filter.setQuery("g:none");
    CHECK(!filter.matchesBook(&book));
    book.clearGenres();
    CHECK(filter.matchesBook(&book));
    filter.setQuery("g:fantasy | r>4");
    CHECK(filter.matchesGenre("Fantasy"));
    filter.setQuery("fr:all");
    book.setDateFinishedReading("31.02.2024");
    CHECK(!filter.matchesBook(&book));
    book.setDateFinishedReading("29.02.2024");
    CHECK(filter.matchesBook(&book));
    filter.setQuery("  | \t ");
    CHECK(!filter.isActive() && filter.matchesBook(&book));
    CHECK(!filter.matchesBook(nullptr));
}

void ValidationChecks(const fs::path&)
{
    CHECK(DateUtils::IsValidDateDDMMYYYY("29.02.2024"));
    CHECK(!DateUtils::IsValidDateDDMMYYYY("29.02.2023"));
    CHECK(Validation::IsValidPublishedDate("2024-02-29"));
    CHECK(!Validation::IsValidPublishedDate("2023-02-29"));
    CHECK(!Validation::IsValidPublishedDate("2024-04-31"));
    float rating = 3;
    CHECK(Validation::TryParseRating(" 4.5 ", rating) && rating == 4.5f);
    for (const auto& input : {"nan", "inf", "4junk", "-1", "6"}) {
        CHECK(!Validation::TryParseRating(input, rating));
    }
    int pages = 0;
    CHECK(Validation::TryParsePageCount(" 321 ", pages) && pages == 321);
    CHECK(!Validation::TryParsePageCount("999999999999999999999", pages));
}

void Paths(const fs::path& root)
{
    SetEnvironment("LOCALAPPDATA", root / "local");
    SetEnvironment("XDG_CONFIG_HOME", root / "config");
    SetEnvironment("XDG_DATA_HOME", root / "data");
#ifdef _WIN32
    CHECK(AppPaths::ConfigFile() == root / "local" / "NookLink" / "settings.conf");
    CHECK(AppPaths::DefaultLibraryFile() == root / "local" / "NookLink" / "library.json");
#else
    CHECK(AppPaths::ConfigFile() == root / "config" / "nooklink" / "settings.conf");
    CHECK(AppPaths::DefaultLibraryFile() == root / "data" / "nooklink" / "library.json");
#endif
    const auto settings = AppPaths::ConfigFile();
    fs::create_directories(settings.parent_path());
    std::string error;
    CHECK(FileStorage::WriteAtomically(settings, "theme_index=1\n", error));
    CHECK(FileStorage::WriteAtomically(settings, "theme_index=2\n", error));
    CHECK(Read(settings) == "theme_index=2\n");
    CHECK(Read(fs::path(settings.string() + ".bak")) == "theme_index=1\n");
}
}

int main(int argc, char** argv)
{
    try {
        CHECK(argc == 2 || argc == 3);
        TemporaryDirectory temporary;
        // Keep test logs and settings inside disposable test data.
        SetEnvironment("APPDATA", temporary.path);
        SetEnvironment("LOCALAPPDATA", temporary.path);
        SetEnvironment("HOME", temporary.path);
        if (argc == 3 && std::string(argv[1]) == "verify-library") {
            BookManager manager;
            Positions positions;
            CHECK(manager.loadBooksFromFile(argv[2], positions));
            CHECK(!manager.wasRecoveredFromBackup());
            std::cout << "Validated library: " << manager.getBooks().size()
                      << " books, " << positions.size() << " saved positions\n";
            return 0;
        }
        CHECK(argc == 2);
        const std::unordered_map<std::string, void(*)(const fs::path&)> tests = {
            {"roundtrip", Roundtrip}, {"backup", Backup}, {"save_failure", SaveFailure},
            {"locked_target", LockedTarget}, {"recovery", Recovery},
            {"invalid_library", InvalidLibrary}, {"next_id", NextId}, {"lookup", Lookup},
            {"search", Search}, {"validation", ValidationChecks}, {"app_paths", Paths}
        };
        tests.at(argv[1])(temporary.path);
        std::cout << "PASS " << argv[1] << '\n';
        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
