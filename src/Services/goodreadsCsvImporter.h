#pragma once

#include <string>
#include <vector>

#include "book.h"

class GoodreadsCsvImporter {
public:
    bool ImportFromFile(const std::string& csvPath, std::vector<Book>& outBooks, std::string& outError) const;
};
