#include "goodreadsCsvImporter.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <unordered_map>

namespace {

std::string TrimCopy(const std::string& value)
{
    auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char c) { return std::isspace(c); });
    auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) { return std::isspace(c); }).base();
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

std::string NormalizeHeaderKey(const std::string& value)
{
    std::string lowered = ToLowerAsciiCopy(TrimCopy(value));
    std::string normalized;
    normalized.reserve(lowered.size());
    for (char ch : lowered) {
        if (std::isalnum((unsigned char)ch)) {
            normalized.push_back(ch);
        }
    }
    return normalized;
}

void StripUtf8Bom(std::string& text)
{
    if (text.size() >= 3 &&
        (unsigned char)text[0] == 0xEF &&
        (unsigned char)text[1] == 0xBB &&
        (unsigned char)text[2] == 0xBF) {
        text.erase(0, 3);
    }
}

bool ReadCsvRecord(std::istream& in, std::string& outRecord)
{
    outRecord.clear();

    std::string line;
    if (!std::getline(in, line)) {
        return false;
    }

    outRecord = line;

    auto countEffectiveQuotes = [](const std::string& s) {
        int count = 0;
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] != '"') continue;
            if (i + 1 < s.size() && s[i + 1] == '"') {
                ++i;
                continue;
            }
            ++count;
        }
        return count;
    };

    int quoteCount = countEffectiveQuotes(outRecord);
    while ((quoteCount % 2) != 0 && std::getline(in, line)) {
        outRecord += "\n";
        outRecord += line;
        quoteCount += countEffectiveQuotes(line);
    }

    return true;
}

std::vector<std::string> ParseCsvLine(const std::string& line)
{
    std::vector<std::string> cells;
    std::string current;
    bool inQuotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        const char ch = line[i];
        if (ch == '"') {
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                current.push_back('"');
                ++i;
            }
            else {
                inQuotes = !inQuotes;
            }
            continue;
        }

        if (ch == ',' && !inQuotes) {
            cells.push_back(current);
            current.clear();
            continue;
        }

        current.push_back(ch);
    }

    cells.push_back(current);
    return cells;
}

std::string GetCell(const std::vector<std::string>& row, const std::unordered_map<std::string, size_t>& headerIndex, const std::string& column)
{
    const auto it = headerIndex.find(NormalizeHeaderKey(column));
    if (it == headerIndex.end()) {
        return "";
    }

    const size_t idx = it->second;
    if (idx >= row.size()) {
        return "";
    }

    return TrimCopy(row[idx]);
}

int ParseIntSafe(const std::string& text, int fallback = 0)
{
    const std::string trimmed = TrimCopy(text);
    if (trimmed.empty()) {
        return fallback;
    }

    try {
        return std::stoi(trimmed);
    }
    catch (...) {
        return fallback;
    }
}

bool TryConvertDateToDDMMYYYY(const std::string& input, std::string& outDate)
{
    const std::string text = TrimCopy(input);
    if (text.empty()) {
        outDate.clear();
        return true;
    }

    auto parseTwo = [](const std::string& s, size_t pos, int& out) {
        if (pos + 2 > s.size()) return false;
        if (!std::isdigit((unsigned char)s[pos]) || !std::isdigit((unsigned char)s[pos + 1])) return false;
        out = (s[pos] - '0') * 10 + (s[pos + 1] - '0');
        return true;
    };

    auto parseFour = [](const std::string& s, size_t pos, int& out) {
        if (pos + 4 > s.size()) return false;
        for (size_t i = 0; i < 4; ++i) {
            if (!std::isdigit((unsigned char)s[pos + i])) return false;
        }
        out = (s[pos] - '0') * 1000 + (s[pos + 1] - '0') * 100 + (s[pos + 2] - '0') * 10 + (s[pos + 3] - '0');
        return true;
    };

    int day = 0, month = 0, year = 0;

    if (text.size() == 10 && text[4] == '-' && text[7] == '-') {
        if (!parseFour(text, 0, year) || !parseTwo(text, 5, month) || !parseTwo(text, 8, day)) return false;
    }
    else if (text.size() == 10 && text[4] == '/' && text[7] == '/') {
        if (!parseFour(text, 0, year) || !parseTwo(text, 5, month) || !parseTwo(text, 8, day)) return false;
    }
    else if (text.size() == 10 && text[2] == '/' && text[5] == '/') {
        if (!parseTwo(text, 0, month) || !parseTwo(text, 3, day) || !parseFour(text, 6, year)) return false;
    }
    else {
        return false;
    }

    if (year < 1000 || month < 1 || month > 12 || day < 1 || day > 31) {
        return false;
    }

    char buffer[16];
#if defined(_MSC_VER)
    sprintf_s(buffer, "%02d.%02d.%04d", day, month, year);
#else
    std::snprintf(buffer, sizeof(buffer), "%02d.%02d.%04d", day, month, year);
#endif
    outDate = buffer;
    return true;
}

Status ParseStatus(const std::string& exclusiveShelf)
{
    const std::string lowered = ToLowerAsciiCopy(TrimCopy(exclusiveShelf));
    if (lowered == "read") {
        return Status::Read;
    }
    if (lowered == "currently-reading") {
        return Status::Reading;
    }
    return Status::ToRead;
}


} // namespace

bool GoodreadsCsvImporter::ImportFromFile(const std::string& csvPath, std::vector<Book>& outBooks, std::string& outError) const
{
    outBooks.clear();
    outError.clear();

    std::ifstream file(csvPath);
    if (!file.is_open()) {
        outError = "Could not open CSV file.";
        return false;
    }

    std::string headerLine;
    if (!ReadCsvRecord(file, headerLine)) {
        outError = "CSV file is empty.";
        return false;
    }
    StripUtf8Bom(headerLine);

    const std::vector<std::string> headers = ParseCsvLine(headerLine);
    std::unordered_map<std::string, size_t> headerIndex;
    for (size_t i = 0; i < headers.size(); ++i) {
        headerIndex[NormalizeHeaderKey(headers[i])] = i;
    }

    if (!headerIndex.contains("title") || !headerIndex.contains("author")) {
        outError = "CSV does not look like Goodreads export (missing Title/Author columns).";
        return false;
    }

    std::string line;
    while (ReadCsvRecord(file, line)) {
        if (TrimCopy(line).empty()) {
            continue;
        }

        const std::vector<std::string> row = ParseCsvLine(line);

        const std::string title = GetCell(row, headerIndex, "title");
        const std::string author = GetCell(row, headerIndex, "author");
        if (title.empty() || author.empty()) {
            continue;
        }

        const Status status = ParseStatus(GetCell(row, headerIndex, "exclusive shelf"));
        Book book(title, author, status);

        const int pages = ParseIntSafe(GetCell(row, headerIndex, "number of pages"), 0);
        book.setPageCount(pages < 0 ? 0 : pages);

        int rating = ParseIntSafe(GetCell(row, headerIndex, "my rating"), 0);
        if (rating < 0) rating = 0;
        if (rating > 5) rating = 5;
        book.setRating((float)rating);

        std::string published = GetCell(row, headerIndex, "year published");
        if (published.empty()) {
            published = GetCell(row, headerIndex, "original publication year");
        }
        if (!published.empty() && published.size() >= 4) {
            book.setDatePublished(published.substr(0, 4));
        }

        const std::string review = GetCell(row, headerIndex, "my review");
        if (!review.empty()) {
            book.setNotes(review);
        }

        

        std::string dateAdded;
        if (TryConvertDateToDDMMYYYY(GetCell(row, headerIndex, "date added"), dateAdded) && !dateAdded.empty()) {
            book.setDateAdded(dateAdded);
        }

        std::string dateRead;
        if (TryConvertDateToDDMMYYYY(GetCell(row, headerIndex, "date read"), dateRead) && !dateRead.empty()) {
            book.setDateFinishedReading(dateRead);
            if (status == Status::Read && book.getDateStartedReading().empty()) {
                book.setDateStartedReading(dateRead);
            }
        }

        outBooks.push_back(book);
    }

    if (outBooks.empty()) {
        outError = "No valid books found in CSV.";
        return false;
    }

    return true;
}
