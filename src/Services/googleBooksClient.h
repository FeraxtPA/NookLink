#pragma once


#include <string>

struct GoogleBookData {
    bool hasTitle = false;
    std::string title;

    bool hasAuthor = false;
    std::string author;

    bool hasGenres = false;
    std::string genres;

    bool hasRating = false;
    float rating = 0.0f;

    bool hasPageCount = false;
    int pageCount = 0;

    bool hasPublishedDate = false;
    std::string publishedDate;

    bool hasDescription = false;
    std::string description;
};

class GoogleBooksClient {
public:
    bool FetchByIsbn(const std::string& isbn, GoogleBookData& outData, std::string& outError) const;
};
