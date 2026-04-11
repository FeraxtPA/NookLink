#include "uiManager.h"

#include "book.h"
#include "constants.h"
#include "UI/button.h"
#include "UI/label.h"
#include "UI/panel.h"
#include "uiManager_internal.h"

#include <algorithm>
#include <format>

namespace {
std::string TrimCopy(std::string value)
{
    const size_t first = value.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    const size_t last = value.find_last_not_of(" \t\n\r");
    return value.substr(first, last - first + 1);
}

std::string FormatDetailSection(const std::string& icon, const std::string& heading, const std::string& body)
{
    return icon + std::string("  ") + heading + "\n" + body + "\n" + NookConst::Text::kDetailsDivider + "\n\n";
}
}

void UIManager::OpenBookDetails(Book* book)
{
    if (!book || !m_BookDetailsPanel) return;

    m_CurrentDetailsBook = *book;
    m_HasCurrentDetailsBook = true;

    std::string genreStr;
    const auto& genres = book->getGenres();
    for (size_t i = 0; i < genres.size(); ++i) {
        genreStr += genres[i];
        if (i < genres.size() - 1) genreStr += ", ";
    }
    if (genreStr.empty()) {
        genreStr = "-";
    }

    const std::string notes = TrimCopy(book->getNotes()).empty() ? std::string("No notes yet.") : book->getNotes();
    const std::string status = statusToString(book->getStatus());
    const std::string stars = Book::ratingToStars(book->getRating());
    const std::string pageCount = book->getPageCount() > 0 ? std::to_string(book->getPageCount()) : std::string("-");
    const std::string published = book->getDatePublished().empty() ? std::string("-") : book->getDatePublished();
    const std::string datesBody =
        "Added    : " + (book->getDateAdded().empty() ? std::string("-") : book->getDateAdded()) + "\n" +
        "Started  : " + (book->getDateStartedReading().empty() ? std::string("-") : book->getDateStartedReading()) + "\n" +
        "Finished : " + (book->getDateFinishedReading().empty() ? std::string("-") : book->getDateFinishedReading());

    std::string fullText;
    fullText.reserve(1024);
    fullText += FormatDetailSection("✦", "Title", book->getTitle());
    fullText += FormatDetailSection("✎", "Author", book->getAuthor());
    fullText += FormatDetailSection("☑", "Status", status);
    fullText += FormatDetailSection("⧉", "Pages", pageCount);
    fullText += FormatDetailSection("⌛", "Published", published);
    fullText += FormatDetailSection("★", "Rating", std::format("{}  ({:.1f}/5.0)", stars, book->getRating()));
    fullText += FormatDetailSection("✧", "Dates", datesBody);
    fullText += FormatDetailSection("➤", "Genres", genreStr);
    fullText += std::string("➜  Notes\n") + notes;

    m_DetailsText->SetText(fullText);
    if (m_BookDetailsPanel) m_BookDetailsPanel->SetTitle("Book Info");
    if (m_DetailsEditBtn) m_DetailsEditBtn->SetVisible(true);
    if (m_DetailsCloseBtn) m_DetailsCloseBtn->SetText("Close");

    m_BookDetailsPanel->SetVisible(true);
}

void UIManager::OpenGenreDetails(const std::string& genreName, int connectedBooks, const std::vector<std::string>& sampleTitles)
{
    if (!m_BookDetailsPanel || !m_DetailsText) return;

    m_HasCurrentDetailsBook = false;

    std::string preview;
    if (sampleTitles.empty()) {
        preview = "No books currently tagged with this genre.";
    }
    else {
        const size_t previewCount = std::min<size_t>(sampleTitles.size(), 6);
        for (size_t i = 0; i < previewCount; ++i) {
            preview += std::format("{:>2}. {}\n", i + 1, sampleTitles[i]);
        }
        if (sampleTitles.size() > previewCount) {
            preview += std::format("... and {} more", sampleTitles.size() - previewCount);
        }
    }

    std::string fullText;
    fullText.reserve(720);
    fullText += FormatDetailSection("➤", "Genre", genreName);
    fullText += FormatDetailSection("✧", "Connected books", std::to_string(connectedBooks));
    fullText += "▶  Preview\n" + preview;

    m_DetailsText->SetText(fullText);
    if (m_BookDetailsPanel) m_BookDetailsPanel->SetTitle("Genre Info");
    if (m_DetailsEditBtn) m_DetailsEditBtn->SetVisible(false);
    if (m_DetailsCloseBtn) m_DetailsCloseBtn->SetText("Close");
    m_BookDetailsPanel->SetVisible(true);
}

Rectangle UIManager::GetSettingsHelpRect() const
{
    if (!m_SettingsPanel || !m_SettingsPanel->IsVisible() || !m_SettingsHelpExpanded) {
        return Rectangle{ 0.0f, 0.0f, 0.0f, 0.0f };
    }

    const Rectangle panel = m_SettingsPanel->GetBounds();
    const float screenPad = NookConst::Layout::kSettingsHelpScreenPadding;
    const float panelGap = NookConst::Layout::kSettingsHelpPanelGap;
    const float desiredWidth = NookConst::Layout::kSettingsHelpDesiredWidth;
    const float minViableWidth = NookConst::Layout::kSettingsHelpMinWidth;
    const float boxHeight = NookConst::Layout::kSettingsHelpHeight;

    const float rightSpace = (float)m_ScreenWidth - screenPad - (panel.x + panel.width + panelGap);
    const float leftSpace = panel.x - panelGap - screenPad;

    bool placeRight = rightSpace >= leftSpace;
    float availableWidth = placeRight ? rightSpace : leftSpace;

    if (availableWidth < minViableWidth) {
        placeRight = !placeRight;
        availableWidth = placeRight ? rightSpace : leftSpace;
    }

    if (availableWidth < minViableWidth) {
        return Rectangle{ 0.0f, 0.0f, 0.0f, 0.0f };
    }

    const float boxWidth = std::min(desiredWidth, availableWidth);
    const float boxX = placeRight
        ? (panel.x + panel.width + panelGap)
        : (panel.x - panelGap - boxWidth);

    float boxY = panel.y + NookConst::Layout::kSettingsHelpTopOffset;
    const float minY = UiMetrics::kToolbarShellHeight + screenPad;
    const float maxY = (float)m_ScreenHeight - boxHeight - screenPad;
    boxY = std::clamp(boxY, minY, maxY);

    return Rectangle{ boxX, boxY, boxWidth, boxHeight };
}
