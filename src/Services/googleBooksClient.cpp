#include "googleBooksClient.h"


#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>

#include <string>

#if __has_include(<curl/curl.h>)
#define NOOKLINK_HAS_CURL 1
#include <curl/curl.h>
#ifdef _MSC_VER
#pragma comment(lib, "libcurl.lib")
#endif
#else
#define NOOKLINK_HAS_CURL 0
#endif

namespace {

std::string TrimCopy(const std::string& input)
{
    auto begin = std::find_if_not(input.begin(), input.end(), [](unsigned char c) { return std::isspace(c); });
    auto end = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char c) { return std::isspace(c); }).base();
    if (begin >= end) {
        return "";
    }

    return std::string(begin, end);
}

std::string ToLowerAsciiCopy(std::string value)
{
    for (char& ch : value) {
        ch = (char)std::tolower((unsigned char)ch);
    }
    return value;
}

bool IsLikelyGenreCategory(const std::string& category)
{
    const std::string trimmed = TrimCopy(category);
    if (trimmed.empty()) {
        return false;
    }

    const std::string lowered = ToLowerAsciiCopy(trimmed);

    const char* blockedTokens[] = {
        "fictitious character",
        "fictional character",
        "characters and characteristics",
        "-- characters",
        "personality"
    };
    for (const char* token : blockedTokens) {
        if (lowered.find(token) != std::string::npos) {
            return false;
        }
    }

    const char* genreHints[] = {
        "fiction", "nonfiction", "non-fiction", "fantasy", "romance", "mystery",
        "thriller", "horror", "science fiction", "sci-fi", "history", "historical",
        "poetry", "drama", "adventure", "crime", "young adult", "children",
        "comics", "graphic novels", "manga", "classics", "dystopian", "memoir"
    };
    for (const char* token : genreHints) {
        if (lowered.find(token) != std::string::npos) {
            return true;
        }
    }

    if (trimmed.find('(') == std::string::npos && trimmed.find(')') == std::string::npos &&
        trimmed.find(',') == std::string::npos && trimmed.find("--") == std::string::npos) {
        return true;
    }

    return false;
}

std::string JoinFilteredGenreArray(const nlohmann::json& value)
{
    if (!value.is_array()) {
        return "";
    }

    std::string joined;
    for (const auto& item : value) {
        if (!item.is_string()) {
            continue;
        }

        const std::string category = item.get<std::string>();
        if (!IsLikelyGenreCategory(category)) {
            continue;
        }

        if (!joined.empty()) {
            joined += ", ";
        }
        joined += category;
    }

    return joined;
}

std::string NormalizeIsbn(const std::string& isbn)
{
    std::string normalized;
    normalized.reserve(isbn.size());
    for (char ch : isbn) {
        if (std::isdigit((unsigned char)ch) || ch == 'X' || ch == 'x') {
            normalized.push_back((char)std::toupper((unsigned char)ch));
        }
    }
    return normalized;
}

bool TryReadApiKeyFromEnvFile(std::string& outKey)
{
    namespace fs = std::filesystem;

    fs::path current = fs::current_path();
    for (int i = 0; i < 8; ++i) {
        const fs::path envPath = current / ".env";
        if (fs::exists(envPath)) {
            std::ifstream file(envPath);
            if (!file.is_open()) {
                return false;
            }

            std::string line;
            while (std::getline(file, line)) {
                const std::string trimmed = TrimCopy(line);
                if (trimmed.empty() || trimmed.starts_with('#')) {
                    continue;
                }

                const size_t eqPos = trimmed.find('=');
                if (eqPos == std::string::npos) {
                    continue;
                }

                const std::string key = TrimCopy(trimmed.substr(0, eqPos));
                if (key != "GOOGLE_BOOKS_API_KEY") {
                    continue;
                }

                outKey = TrimCopy(trimmed.substr(eqPos + 1));
                if (!outKey.empty()) {
                    return true;
                }
            }
            return false;
        }

        if (!current.has_parent_path()) {
            break;
        }
        current = current.parent_path();
    }

    return false;
}

bool TryGetApiKey(std::string& outKey)
{
#if defined(_WIN32)
    char* envValue = nullptr;
    size_t valueLength = 0;
    if (_dupenv_s(&envValue, &valueLength, "GOOGLE_BOOKS_API_KEY") == 0 && envValue != nullptr) {
        outKey = TrimCopy(envValue);
        free(envValue);
        if (!outKey.empty()) {
            return true;
        }
    }
#else
    if (const char* env = std::getenv("GOOGLE_BOOKS_API_KEY")) {
        outKey = TrimCopy(env);
        if (!outKey.empty()) {
            return true;
        }
    }
#endif

    return TryReadApiKeyFromEnvFile(outKey);
}

std::string JoinStringArray(const nlohmann::json& value)
{
    if (!value.is_array()) {
        return "";
    }

    std::string joined;
    for (const auto& item : value) {
        if (!item.is_string()) {
            continue;
        }

        if (!joined.empty()) {
            joined += ", ";
        }
        joined += item.get<std::string>();
    }

    return joined;
}

#if NOOKLINK_HAS_CURL
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    const size_t totalSize = size * nmemb;
    std::string* target = static_cast<std::string*>(userp);
    target->append(static_cast<const char*>(contents), totalSize);
    return totalSize;
}
#endif

} // namespace

bool GoogleBooksClient::FetchByIsbn(const std::string& isbn, GoogleBookData& outData, std::string& outError) const
{
    const std::string normalizedIsbn = NormalizeIsbn(isbn);
    if (normalizedIsbn.empty()) {
        outError = "ISBN is empty.";
        return false;
    }

    if (normalizedIsbn.size() != 10 && normalizedIsbn.size() != 13) {
        outError = "ISBN must have 10 or 13 characters.";
        return false;
    }

    std::string apiKey;
    if (!TryGetApiKey(apiKey)) {
        outError = "Missing GOOGLE_BOOKS_API_KEY in environment or .env file.";
        return false;
    }

#if !NOOKLINK_HAS_CURL
    outError = "libcurl is not available in this build.";
    return false;
#else
    CURL* curl = curl_easy_init();
    if (!curl) {
        outError = "Failed to initialize libcurl.";
        return false;
    }

    std::string response;
    const std::string url = "https://www.googleapis.com/books/v1/volumes?q=isbn:" + normalizedIsbn + "&key=" + apiKey;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 12L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "NookLink/1.0");

    const CURLcode code = curl_easy_perform(curl);
    if (code != CURLE_OK) {
        outError = curl_easy_strerror(code);
        curl_easy_cleanup(curl);
        return false;
    }

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(curl);

    if (httpCode != 200) {
        outError = "Google Books API returned HTTP " + std::to_string(httpCode) + ".";
        return false;
    }

    nlohmann::json root;
    try {
        root = nlohmann::json::parse(response);
    }
    catch (...) {
        outError = "Invalid JSON response from Google Books API.";
        return false;
    }

    if (!root.contains("items") || !root["items"].is_array() || root["items"].empty()) {
        outError = "No book found for this ISBN.";
        return false;
    }

    const nlohmann::json& volumeInfo = root["items"][0].value("volumeInfo", nlohmann::json::object());

    outData = GoogleBookData{};

    if (const auto it = volumeInfo.find("title"); it != volumeInfo.end() && it->is_string()) {
        outData.title = it->get<std::string>();
        outData.hasTitle = !outData.title.empty();
    }

    if (const auto it = volumeInfo.find("authors"); it != volumeInfo.end()) {
        outData.author = JoinStringArray(*it);
        outData.hasAuthor = !outData.author.empty();
    }

    if (const auto it = volumeInfo.find("categories"); it != volumeInfo.end()) {
        outData.genres = JoinFilteredGenreArray(*it);
        outData.hasGenres = !outData.genres.empty();
    }

    if (const auto it = volumeInfo.find("averageRating"); it != volumeInfo.end() && it->is_number()) {
        outData.rating = std::clamp(it->get<float>(), 0.0f, 5.0f);
        outData.hasRating = true;
    }

    if (const auto it = volumeInfo.find("pageCount"); it != volumeInfo.end() && it->is_number_integer()) {
        const int parsedPages = it->get<int>();
        outData.pageCount = parsedPages < 0 ? 0 : parsedPages;
        outData.hasPageCount = outData.pageCount > 0;
    }

    if (const auto it = volumeInfo.find("publishedDate"); it != volumeInfo.end() && it->is_string()) {
        outData.publishedDate = TrimCopy(it->get<std::string>());
        outData.hasPublishedDate = !outData.publishedDate.empty();
    }

    if (const auto it = volumeInfo.find("description"); it != volumeInfo.end() && it->is_string()) {
        outData.description = it->get<std::string>();
        outData.hasDescription = !outData.description.empty();
    }

    if (!outData.hasTitle && !outData.hasAuthor && !outData.hasGenres && !outData.hasRating &&
        !outData.hasPageCount && !outData.hasPublishedDate && !outData.hasDescription) {
        outError = "Book found but no usable fields were returned.";
        return false;
    }

    return true;
#endif
}
