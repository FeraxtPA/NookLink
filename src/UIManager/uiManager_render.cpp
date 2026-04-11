// UI rendering implementation.
// Handles drawing of all UI components and visual elements.


#include "uiManager.h"

#include "UI/widget.h"
#include "UI/panel.h"
#include "UI/label.h"
#include "colors.h"
#include "graphManager.h"
#include "textRenderer.h"
#include "uiManager_internal.h"

#include <algorithm>
#include <format>

void UIManager::Draw(Vector2 mousePos, GraphManager* graphRenderer, const BookManager& bookManager, TextRenderer* textRenderer) const
{
    if (!textRenderer) return;

    const Rectangle toolbarRect = {
        UiMetrics::kToolbarShellMargin,
        UiMetrics::kToolbarShellMargin,
        static_cast<float>(m_ScreenWidth) - (UiMetrics::kToolbarShellMargin * 2.0f),
        UiMetrics::kToolbarShellHeight
    };
    DrawRectangleRounded(toolbarRect, 0.16f, 16, Fade(NookCol::UI_SHELL, 0.98f));
    DrawRectangleRoundedLinesEx(toolbarRect, 0.16f, 16, 2.0f, Fade(NookCol::UI_BORDER_SOFT, 0.50f));
    DrawRectangleRounded({ toolbarRect.x + 1.0f, toolbarRect.y + 1.0f, toolbarRect.width - 2.0f, 28.0f }, 0.12f, 16, Fade(WHITE, 0.04f));

    textRenderer->DrawSimpleText("Library", { UiMetrics::kToolbarLibraryX, UiMetrics::kToolbarLabelY }, UiMetrics::kToolbarLabelFontSize, Fade(NookCol::UI_TEXT, 0.90f));
    textRenderer->DrawSimpleText("Search", { m_ScreenWidth / 2.0f - UiMetrics::kToolbarSearchHalfLabelWidth, UiMetrics::kToolbarLabelY }, UiMetrics::kToolbarLabelFontSize, Fade(NookCol::UI_TEXT, 0.90f));
    textRenderer->DrawSimpleText("Actions", { (float)m_ScreenWidth - UiMetrics::kToolbarActionsInset, UiMetrics::kToolbarLabelY }, UiMetrics::kToolbarLabelFontSize, Fade(NookCol::UI_TEXT, 0.90f));

    const std::string multiText = m_MultiSelectIndicatorActive
        ? ("Multi-select ON (" + std::to_string(m_MultiSelectIndicatorCount) + ")")
        : "Multi-select OFF";
    textRenderer->DrawSimpleText(
        multiText,
        { (float)m_ScreenWidth - 360.0f, UiMetrics::kToolbarLabelY },
        UiMetrics::kToolbarLabelFontSize,
        m_MultiSelectIndicatorActive ? Fade(NookCol::UI_ACCENT, 0.95f) : Fade(NookCol::UI_TEXT_MUTED, 0.85f));

    if (m_SearchFocusIndicatorTotal > 0 && m_SearchFocusIndicatorIndex > 0) {
        const std::string indicator = std::to_string(m_SearchFocusIndicatorIndex) + "/" + std::to_string(m_SearchFocusIndicatorTotal);
        textRenderer->DrawSimpleText(
            indicator,
            { m_ScreenWidth / 2.0f + UiMetrics::kToolbarSearchHalfLabelWidth + 22.0f, UiMetrics::kToolbarLabelY },
            UiMetrics::kToolbarLabelFontSize,
            Fade(NookCol::UI_ACCENT_SOFT, 0.95f));
    }

    textRenderer->DrawSimpleText(std::to_string(GetFPS()), { 10, (float)m_ScreenHeight - 20 }, 20, NookCol::UI_ACCENT_SOFT);

    if (m_AnalyticsPanel && m_AnalyticsPanel->IsVisible()) {
        LayoutAnalyticsLeftLabels();
    }

    for (auto& w : m_Widgets) w->Draw(textRenderer);

    DrawAnalyticsCharts(textRenderer);

    if (m_NodeContextMenuVisible) {
        DrawRectangleRounded(m_NodeContextMenuBounds, 0.16f, 8, NookCol::UI_PANEL_ALT);
        DrawRectangleRoundedLinesEx(m_NodeContextMenuBounds, 0.16f, 8, 2.0f, NookCol::UI_BORDER_SOFT);

        textRenderer->DrawSimpleText("Quick Actions", { m_NodeContextMenuBounds.x + 10.0f, m_NodeContextMenuBounds.y + 8.0f }, 20, NookCol::UI_TEXT);

        const Rectangle listRect = {
            m_NodeContextMenuBounds.x + 6.0f,
            m_NodeContextMenuBounds.y + 34.0f,
            m_NodeContextMenuBounds.width - 12.0f,
            m_NodeContextMenuBounds.height - 40.0f
        };

        if (m_NodeContextNodeType == NodeType::Book) {
            const float itemHeight = listRect.height / 3.0f;
            const char* labels[3] = {
                "Edit book",
                "Delete book",
                m_NodeContextNodeLocked ? "Unlock node" : "Lock node"
            };

            for (int i = 0; i < 3; ++i) {
                Rectangle itemRect = {
                    listRect.x,
                    listRect.y + itemHeight * (float)i,
                    listRect.width,
                    itemHeight
                };
                const bool hovered = (i == m_NodeContextHoverIndex);
                DrawRectangleRounded(itemRect, 0.12f, 6, hovered ? NookCol::UI_PANEL_HOVER : NookCol::UI_PANEL);
                textRenderer->DrawSimpleText(labels[i], { itemRect.x + 10.0f, itemRect.y + 7.0f }, 20, NookCol::UI_TEXT);
            }
        }
        else {
            Rectangle itemRect = { listRect.x, listRect.y, listRect.width, listRect.height };
            const bool hovered = (m_NodeContextHoverIndex == 0);
            DrawRectangleRounded(itemRect, 0.12f, 6, hovered ? NookCol::UI_PANEL_HOVER : NookCol::UI_PANEL);
            textRenderer->DrawSimpleText(m_NodeContextNodeLocked ? "Unlock node" : "Lock node", { itemRect.x + 10.0f, itemRect.y + 7.0f }, 20, NookCol::UI_TEXT);
        }
    }

    DrawHelpText(textRenderer);

    if (graphRenderer != nullptr) {
        std::string count = "Nodes: " + std::to_string(graphRenderer->getNodes().size());
        textRenderer->DrawSimpleText(count, { 10, (float)m_ScreenHeight - 40 }, 20, NookCol::UI_TEXT_MUTED);
    }

    DrawNotification(textRenderer);
}

void UIManager::LayoutAnalyticsLeftLabels() const
{
    if (!m_AnalyticsPanel) {
        return;
    }

    const Rectangle panel = m_AnalyticsPanel->GetBounds();
    const float outerPad = 18.0f;
    const float splitGap = 14.0f;
    const float contentTop = panel.y + 48.0f;
    const float contentHeight = std::max(120.0f, panel.height - 62.0f - 70.0f);
    const float halfWidth = (panel.width - outerPad * 2.0f - splitGap) * 0.5f;
    const Rectangle leftHalf = { panel.x + outerPad, contentTop, halfWidth, contentHeight };

    const float x = leftHalf.x + 10.0f;
    const float w = leftHalf.width - 18.0f;
    const float gap = 8.0f;

    const float overviewH = NookConst::Render::kAnalyticsOverviewVisibleLines * NookConst::Render::kAnalyticsOverviewLineHeight;
    const float listH = NookConst::Render::kAnalyticsListVisibleLines * NookConst::Render::kAnalyticsListLineHeight;
    const float compactH = NookConst::Render::kAnalyticsCompactVisibleLines * NookConst::Render::kAnalyticsCompactLineHeight;

    float y = leftHalf.y + 10.0f;

    if (m_AnalyticsOverviewLabel) {
        m_AnalyticsOverviewLabel->SetBounds({ x, y, w, overviewH });
    }
    y += overviewH + gap;
    if (m_AnalyticsTopGenresListLabel) {
        m_AnalyticsTopGenresListLabel->SetBounds({ x, y, w, listH });
    }
    y += listH + gap;
    if (m_AnalyticsTopAuthorsListLabel) {
        m_AnalyticsTopAuthorsListLabel->SetBounds({ x, y, w, listH });
    }
    y += listH + gap;
    if (m_AnalyticsTopRatedListLabel) {
        const float remainingH = std::max(60.0f, (leftHalf.y + leftHalf.height) - y - (compactH * 2.0f) - (gap * 2.0f) - 4.0f);
        m_AnalyticsTopRatedListLabel->SetBounds({ x, y, w, std::min(listH, remainingH) });
        y += std::min(listH, remainingH) + gap;
    }
    if (m_AnalyticsPagesLabel) {
        m_AnalyticsPagesLabel->SetBounds({ x, y, w, compactH });
        y += compactH + gap;
    }
    if (m_AnalyticsPublicationLabel) {
        const float remainingH = std::max(48.0f, (leftHalf.y + leftHalf.height) - y - 4.0f);
        m_AnalyticsPublicationLabel->SetBounds({ x, y, w, std::min(compactH, remainingH) });
    }
}

void UIManager::DrawAnalyticsCharts(TextRenderer* renderer) const
{
    if (!renderer || !m_AnalyticsPanel || !m_AnalyticsPanel->IsVisible()) {
        return;
    }

    const Rectangle panel = m_AnalyticsPanel->GetBounds();
    const float outerPad = 18.0f;
    const float splitGap = 14.0f;
    const float contentTop = panel.y + 48.0f;
    const float contentHeight = std::max(120.0f, panel.height - 62.0f - 70.0f);
    const float halfWidth = (panel.width - outerPad * 2.0f - splitGap) * 0.5f;

    const Rectangle leftHalf = { panel.x + outerPad, contentTop, halfWidth, contentHeight };
    const Rectangle rightHalf = { leftHalf.x + leftHalf.width + splitGap, contentTop, halfWidth, contentHeight };

    const float splitX = leftHalf.x + leftHalf.width + splitGap * 0.5f;
    DrawLineEx(
        { splitX, leftHalf.y + 4.0f },
        { splitX, leftHalf.y + leftHalf.height - 4.0f },
        2.0f,
        Fade(NookCol::UI_BORDER_SOFT, 0.7f));

    DrawRectangleRounded(rightHalf, 0.08f, 10, Fade(NookCol::UI_PANEL_ALT, 0.92f));
    DrawRectangleRoundedLinesEx(rightHalf, 0.08f, 10, 1.0f, Fade(NookCol::UI_BORDER_SOFT, 0.7f));

    const int total = std::max(1, m_AnalyticsTotalBooks);
    const float rx = rightHalf.x + 16.0f;
    const float rw = rightHalf.width - 32.0f;

    if (m_AnalyticsRightPanelMode == AnalyticsRightPanelMode::StatusPie) {
        renderer->DrawSimpleText("Status panel (Pie)", { rightHalf.x + 14.0f, rightHalf.y + 12.0f }, 22, NookCol::UI_TEXT);
        const float radius = std::min(rightHalf.width, rightHalf.height) * 0.24f;
        const Vector2 center = { rightHalf.x + rightHalf.width * 0.35f, rightHalf.y + rightHalf.height * 0.50f };

        float angleStart = -90.0f;
        auto drawSlice = [&](int count, Color color) {
            const float sweep = 360.0f * (static_cast<float>(count) / static_cast<float>(total));
            if (sweep <= 0.0f) return;
            DrawCircleSector(center, radius, angleStart, angleStart + sweep, 64, Fade(color, 0.95f));
            angleStart += sweep;
        };

        drawSlice(m_AnalyticsToReadCount, NookCol::TO_READ);
        drawSlice(m_AnalyticsReadingCount, NookCol::CURRENTLY_READING);
        drawSlice(m_AnalyticsReadCount, NookCol::READ);
        DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), radius, Fade(NookCol::UI_BORDER_SOFT, 0.9f));

        renderer->DrawSimpleText(std::format("To Read: {}", m_AnalyticsToReadCount), { rightHalf.x + rightHalf.width * 0.62f, rightHalf.y + 92.0f }, 20, NookCol::UI_TEXT);
        renderer->DrawSimpleText(std::format("Reading: {}", m_AnalyticsReadingCount), { rightHalf.x + rightHalf.width * 0.62f, rightHalf.y + 124.0f }, 20, NookCol::UI_TEXT);
        renderer->DrawSimpleText(std::format("Read: {}", m_AnalyticsReadCount), { rightHalf.x + rightHalf.width * 0.62f, rightHalf.y + 156.0f }, 20, NookCol::UI_TEXT);
    }
    else if (m_AnalyticsRightPanelMode == AnalyticsRightPanelMode::RatingProgress) {
        renderer->DrawSimpleText("Rating panel", { rightHalf.x + 14.0f, rightHalf.y + 12.0f }, 22, NookCol::UI_TEXT);

        renderer->DrawSimpleText(std::format("Avg rating: {:.2f}/5", m_AnalyticsAvgRating), { rx, rightHalf.y + 58.0f }, 20, NookCol::UI_TEXT_MUTED);
        DrawRectangleRounded({ rx, rightHalf.y + 86.0f, rw, 24.0f }, 0.35f, 8, Fade(NookCol::UI_PANEL_HOVER, 0.65f));
        DrawRectangleRounded({ rx, rightHalf.y + 86.0f, rw * std::clamp(m_AnalyticsAvgRating / 5.0f, 0.0f, 1.0f), 24.0f }, 0.35f, 8, Fade(NookCol::UI_ACCENT, 0.95f));

        const float yearRatio = std::clamp(static_cast<float>(m_AnalyticsFinishedThisYear) / static_cast<float>(total), 0.0f, 1.0f);
        renderer->DrawSimpleText(std::format("Finished in year: {}", m_AnalyticsFinishedThisYear), { rx, rightHalf.y + 134.0f }, 20, NookCol::UI_TEXT_MUTED);
        DrawRectangleRounded({ rx, rightHalf.y + 162.0f, rw, 24.0f }, 0.35f, 8, Fade(NookCol::UI_PANEL_HOVER, 0.65f));
        DrawRectangleRounded({ rx, rightHalf.y + 162.0f, rw * yearRatio, 24.0f }, 0.35f, 8, Fade(NookCol::UI_ACCENT_SOFT, 0.95f));

        const float ratedRatio = std::clamp(static_cast<float>(m_AnalyticsRatedCount) / static_cast<float>(total), 0.0f, 1.0f);
        renderer->DrawSimpleText(std::format("Rated books: {}", m_AnalyticsRatedCount), { rx, rightHalf.y + 210.0f }, 20, NookCol::UI_TEXT_MUTED);
        DrawRectangleRounded({ rx, rightHalf.y + 238.0f, rw, 24.0f }, 0.35f, 8, Fade(NookCol::UI_PANEL_HOVER, 0.65f));
        DrawRectangleRounded({ rx, rightHalf.y + 238.0f, rw * ratedRatio, 24.0f }, 0.35f, 8, Fade(NookCol::UI_BORDER, 0.95f));
    }
    else if (m_AnalyticsRightPanelMode == AnalyticsRightPanelMode::RatingHistogram) {
        renderer->DrawSimpleText("Rating histogram", { rightHalf.x + 14.0f, rightHalf.y + 12.0f }, 22, NookCol::UI_TEXT);

        const int maxBin = std::max({ 1, m_AnalyticsRatingBins[0], m_AnalyticsRatingBins[1], m_AnalyticsRatingBins[2], m_AnalyticsRatingBins[3], m_AnalyticsRatingBins[4] });
        const float chartX = rx;
        const float chartY = rightHalf.y + 74.0f;
        const float chartW = rw;
        const float chartH = std::max(120.0f, rightHalf.height - 150.0f);
        const float slotW = chartW / 5.0f;

        for (int i = 0; i < 5; ++i) {
            const float barW = slotW * 0.62f;
            const float bx = chartX + slotW * i + (slotW - barW) * 0.5f;
            const float ratio = static_cast<float>(m_AnalyticsRatingBins[static_cast<size_t>(i)]) / static_cast<float>(maxBin);
            const float bh = chartH * ratio;
            const float by = chartY + chartH - bh;

            DrawRectangleRounded({ bx, chartY, barW, chartH }, 0.15f, 6, Fade(NookCol::UI_PANEL_HOVER, 0.45f));
            DrawRectangleRounded({ bx, by, barW, bh }, 0.15f, 6, Fade(NookCol::UI_ACCENT, 0.95f));

            renderer->DrawSimpleText(std::format("{}-{}", i, i + 1), { bx + 1.0f, chartY + chartH + 8.0f }, 18, NookCol::UI_TEXT_MUTED);
            renderer->DrawSimpleText(std::to_string(m_AnalyticsRatingBins[static_cast<size_t>(i)]), { bx + 4.0f, by - 20.0f }, 18, NookCol::UI_TEXT);
        }
    }
    else if (m_AnalyticsRightPanelMode == AnalyticsRightPanelMode::ReadingMomentum) {
        renderer->DrawSimpleText("Reading momentum", { rightHalf.x + 14.0f, rightHalf.y + 12.0f }, 22, NookCol::UI_TEXT);

        const float gx = rx;
        const float gy = rightHalf.y + 78.0f;
        const float gw = rw;

        auto drawGauge = [&](const char* title, float y, float ratio, Color color, const std::string& detail) {
            DrawRectangleRounded({ gx, y, gw, 24.0f }, 0.35f, 8, Fade(NookCol::UI_PANEL_HOVER, 0.55f));
            DrawRectangleRounded({ gx, y, gw * std::clamp(ratio, 0.0f, 1.0f), 24.0f }, 0.35f, 8, Fade(color, 0.95f));
            renderer->DrawSimpleText(std::string(title) + "  " + detail, { gx + 8.0f, y + 3.0f }, 19, NookCol::UI_TEXT);
        };

        const float readRatio = static_cast<float>(m_AnalyticsReadCount) / static_cast<float>(total);
        const float readingRatio = static_cast<float>(m_AnalyticsReadingCount) / static_cast<float>(total);
        const float monthlyRatio = static_cast<float>(m_AnalyticsFinishedThisMonth) / static_cast<float>(std::max(1, m_AnalyticsReadCount));
        const float yearlyRatio = static_cast<float>(m_AnalyticsFinishedThisYear) / static_cast<float>(std::max(1, m_AnalyticsTotalBooks));

        drawGauge("Read share", gy + 0.0f, readRatio, NookCol::READ, std::format("{:.0f}%", readRatio * 100.0f));
        drawGauge("Reading share", gy + 44.0f, readingRatio, NookCol::CURRENTLY_READING, std::format("{:.0f}%", readingRatio * 100.0f));
        drawGauge("Monthly pace", gy + 88.0f, monthlyRatio, NookCol::UI_ACCENT, std::format("{} done this month", m_AnalyticsFinishedThisMonth));
        drawGauge("Year progress", gy + 132.0f, yearlyRatio, NookCol::UI_ACCENT_SOFT, std::format("{} done this year", m_AnalyticsFinishedThisYear));
    }
    else if (m_AnalyticsRightPanelMode == AnalyticsRightPanelMode::AuthorDistribution) {
        renderer->DrawSimpleText("Author distribution", { rightHalf.x + 14.0f, rightHalf.y + 12.0f }, 22, NookCol::UI_TEXT);

        const size_t topN = std::min<size_t>(15, m_AnalyticsTopAuthors.size());
        int maxCount = 1;
        for (size_t i = 0; i < topN; ++i) {
            maxCount = std::max(maxCount, m_AnalyticsTopAuthors[i].second);
        }

        float y = rightHalf.y + 54.0f;
        for (size_t i = 0; i < topN; ++i) {
            const float ratio = static_cast<float>(m_AnalyticsTopAuthors[i].second) / static_cast<float>(maxCount);
            DrawRectangleRounded({ rx, y, rw, 22.0f }, 0.3f, 8, Fade(NookCol::UI_PANEL_HOVER, 0.55f));
            DrawRectangleRounded({ rx, y, rw * ratio, 22.0f }, 0.3f, 8, Fade(NookCol::UI_ACCENT_SOFT, 0.95f));
            renderer->DrawSimpleText(
                std::format("{}. {} ({})", i + 1, m_AnalyticsTopAuthors[i].first, m_AnalyticsTopAuthors[i].second),
                { rx + 6.0f, y + 2.0f },
                18,
                NookCol::UI_TEXT);
            y += 30.0f;
        }

        if (topN == 0) {
            renderer->DrawSimpleText("No authors yet", { rx, rightHalf.y + 60.0f }, 20, NookCol::UI_TEXT_MUTED);
        }
    }
    else if (m_AnalyticsRightPanelMode == AnalyticsRightPanelMode::GenreTreemap) {
        renderer->DrawSimpleText("Genre treemap-like bars", { rightHalf.x + 14.0f, rightHalf.y + 12.0f }, 22, NookCol::UI_TEXT);

        const size_t topN = std::min<size_t>(10, m_AnalyticsTopGenres.size());
        int sumCounts = 0;
        for (size_t i = 0; i < topN; ++i) {
            sumCounts += m_AnalyticsTopGenres[i].second;
        }
        if (sumCounts <= 0) {
            renderer->DrawSimpleText("No genres yet", { rx, rightHalf.y + 60.0f }, 20, NookCol::UI_TEXT_MUTED);
            return;
        }

        const float treemapY = rightHalf.y + 58.0f;
        const float treemapH = std::max(80.0f, rightHalf.height - 120.0f);
        float cursorX = rx;

        for (size_t i = 0; i < topN; ++i) {
            const float ratio = static_cast<float>(m_AnalyticsTopGenres[i].second) / static_cast<float>(sumCounts);
            const float w = (i + 1 == topN) ? (rx + rw - cursorX) : (rw * ratio);
            const Color base = (i % 2 == 0) ? NookCol::UI_ACCENT : NookCol::UI_ACCENT_SOFT;
            DrawRectangleRec({ cursorX, treemapY, w, treemapH }, Fade(base, 0.85f));
            DrawRectangleLinesEx({ cursorX, treemapY, w, treemapH }, 1.0f, Fade(NookCol::UI_BORDER_SOFT, 0.9f));

            if (w > 72.0f) {
                renderer->DrawSimpleText(
                    std::format("{}", m_AnalyticsTopGenres[i].first),
                    { cursorX + 6.0f, treemapY + 8.0f },
                    18,
                    NookCol::UI_TEXT);
                renderer->DrawSimpleText(
                    std::format("{}", m_AnalyticsTopGenres[i].second),
                    { cursorX + 6.0f, treemapY + 30.0f },
                    18,
                    NookCol::UI_TEXT_MUTED);
            }

            cursorX += w;
        }
    }
    else if (m_AnalyticsRightPanelMode == AnalyticsRightPanelMode::PageDistribution) {
        renderer->DrawSimpleText("Page distribution", { rightHalf.x + 14.0f, rightHalf.y + 12.0f }, 22, NookCol::UI_TEXT);

        const size_t binCount = m_AnalyticsPageBins.size();
        if (binCount == 0 || m_AnalyticsPageBinRanges.size() != binCount) {
            renderer->DrawSimpleText("No page data", { rx, rightHalf.y + 60.0f }, 20, NookCol::UI_TEXT_MUTED);
            return;
        }

        int maxBin = 1;
        for (size_t i = 0; i < binCount; ++i) {
            maxBin = std::max(maxBin, m_AnalyticsPageBins[i]);
        }
        const float chartX = rx;
        const float chartY = rightHalf.y + 74.0f;
        const float chartW = rw;
        const float chartH = std::max(120.0f, rightHalf.height - 170.0f);
        const float slotW = chartW / static_cast<float>(binCount);

        for (size_t i = 0; i < binCount; ++i) {
            const float barW = slotW * 0.62f;
            const float bx = chartX + slotW * static_cast<float>(i) + (slotW - barW) * 0.5f;
            const float ratio = static_cast<float>(m_AnalyticsPageBins[i]) / static_cast<float>(maxBin);
            const float bh = chartH * ratio;
            const float by = chartY + chartH - bh;

            DrawRectangleRounded({ bx, chartY, barW, chartH }, 0.15f, 6, Fade(NookCol::UI_PANEL_HOVER, 0.45f));
            DrawRectangleRounded({ bx, by, barW, bh }, 0.15f, 6, Fade(NookCol::UI_ACCENT_SOFT, 0.95f));

            const auto& [startRange, endRange] = m_AnalyticsPageBinRanges[i];
            if (slotW >= 64.0f) {
                renderer->DrawSimpleText(std::format("{}-{}", startRange, endRange), { bx + 1.0f, chartY + chartH + 8.0f }, 17, NookCol::UI_TEXT_MUTED);
            }
            else if (slotW >= 40.0f && (i % 2 == 0 || i + 1 == binCount)) {
                renderer->DrawSimpleText(std::format("{}", startRange), { bx + 2.0f, chartY + chartH + 8.0f }, 16, NookCol::UI_TEXT_MUTED);
            }

            if (slotW >= 34.0f) {
                renderer->DrawSimpleText(std::to_string(m_AnalyticsPageBins[i]), { bx + 6.0f, by - 20.0f }, 18, NookCol::UI_TEXT);
            }
        }

        renderer->DrawSimpleText(std::format("Books with pages: {}", m_AnalyticsBooksWithPages), { rx, chartY + chartH + 44.0f }, 20, NookCol::UI_TEXT_MUTED);
        renderer->DrawSimpleText(std::format("Avg pages: {:.0f} | Bins: {}", m_AnalyticsAvgPages, static_cast<int>(binCount)), { rx, chartY + chartH + 72.0f }, 20, NookCol::UI_TEXT_MUTED);
    }
    else if (m_AnalyticsRightPanelMode == AnalyticsRightPanelMode::PublicationTimeline) {
        renderer->DrawSimpleText("Publication timeline", { rightHalf.x + 14.0f, rightHalf.y + 12.0f }, 22, NookCol::UI_TEXT);

        const size_t n = m_AnalyticsPublishedByDecade.size();
        if (n == 0) {
            renderer->DrawSimpleText("No published-year data", { rx, rightHalf.y + 60.0f }, 20, NookCol::UI_TEXT_MUTED);
            return;
        }

        int maxCount = 1;
        for (const auto& [decade, count] : m_AnalyticsPublishedByDecade) {
            (void)decade;
            maxCount = std::max(maxCount, count);
        }

        const float chartX = rx;
        const float chartY = rightHalf.y + 74.0f;
        const float chartW = rw;
        const float chartH = std::max(120.0f, rightHalf.height - 170.0f);
        const float slotW = chartW / static_cast<float>(n);
        const size_t maxLabels = 16;
        const size_t labelStep = std::max<size_t>(1, (n + maxLabels - 1) / maxLabels);

        for (size_t i = 0; i < n; ++i) {
            const float barW = slotW * 0.60f;
            const float bx = chartX + slotW * static_cast<float>(i) + (slotW - barW) * 0.5f;
            const float ratio = static_cast<float>(m_AnalyticsPublishedByDecade[i].second) / static_cast<float>(maxCount);
            const float bh = chartH * ratio;
            const float by = chartY + chartH - bh;

            DrawRectangleRounded({ bx, chartY, barW, chartH }, 0.15f, 6, Fade(NookCol::UI_PANEL_HOVER, 0.45f));
            DrawRectangleRounded({ bx, by, barW, bh }, 0.15f, 6, Fade(NookCol::UI_ACCENT, 0.95f));

            const bool drawDecadeLabel = (i % labelStep == 0) || (i + 1 == n);
            if (drawDecadeLabel) {
                renderer->DrawSimpleText(std::format("{}s", m_AnalyticsPublishedByDecade[i].first), { bx - 8.0f, chartY + chartH + 8.0f }, 17, NookCol::UI_TEXT_MUTED);
            }
            if (slotW >= 42.0f) {
                renderer->DrawSimpleText(std::to_string(m_AnalyticsPublishedByDecade[i].second), { bx + 4.0f, by - 20.0f }, 17, NookCol::UI_TEXT);
            }
        }

        renderer->DrawSimpleText(std::format("Known years: {}", m_AnalyticsPublishedYearCount), { rx, chartY + chartH + 44.0f }, 20, NookCol::UI_TEXT_MUTED);
        renderer->DrawSimpleText(std::format("Range: {} - {}", m_AnalyticsOldestPublishedYear, m_AnalyticsNewestPublishedYear), { rx, chartY + chartH + 72.0f }, 20, NookCol::UI_TEXT_MUTED);
    }
}

void UIManager::DrawHelpText(TextRenderer* renderer) const
{
    if (!renderer) {
        return;
    }

    const Rectangle helpRect = GetSettingsHelpRect();
    if (helpRect.width <= 0.0f || helpRect.height <= 0.0f) {
        return;
    }

    DrawRectangleRounded(helpRect, 0.12f, 10, NookCol::UI_PANEL_ALT);
    DrawRectangleRoundedLinesEx(helpRect, 0.12f, 10, 2.0f, NookCol::UI_BORDER_SOFT);

    const float lineX = helpRect.x + 14.0f;
    const float lineStartY = helpRect.y + 14.0f;
    const float lineStep = 32.0f;
    renderer->DrawSimpleText("Controls help", { lineX, lineStartY }, 20, NookCol::UI_TEXT);
    renderer->DrawSimpleText("Right-click drag: Move | Shift+Drag: Lock", { lineX, lineStartY + (lineStep * 1.0f) }, 20, NookCol::UI_TEXT_MUTED);
    renderer->DrawSimpleText("Shift+Click: Delete | Double Click: Unlock", { lineX, lineStartY + (lineStep * 2.0f) }, 20, NookCol::UI_TEXT_MUTED);
    renderer->DrawSimpleText("Middle Click: Pan | Scroll: Zoom | Ctrl+RMB: Node menu", { lineX, lineStartY + (lineStep * 3.0f) }, 20, NookCol::UI_TEXT_MUTED);
    renderer->DrawSimpleText("Ctrl+F Search | Ctrl+N Add | Ctrl+, Settings | F1 Help", { lineX, lineStartY + (lineStep * 4.0f) }, 20, NookCol::UI_TEXT_MUTED);
    renderer->DrawSimpleText("Ctrl+S Save | Ctrl+Shift+S Save As | Ctrl+O Load", { lineX, lineStartY + (lineStep * 5.0f) }, 20, NookCol::UI_TEXT_MUTED);
}

void UIManager::ShowNotification(const std::string& message, float duration)
{
    m_NotificationText = message;
    m_NotificationTimer = duration;
}

void UIManager::DrawNotification(TextRenderer* textRenderer) const
{
    if (m_NotificationTimer <= 0.0f || !textRenderer) return;

    float alpha = 1.0f;
    if (m_NotificationTimer < 0.5f) {
        alpha = m_NotificationTimer / 0.5f;
    }

    const int fontSize = UiMetrics::kDefaultFontSize;
    const float textWidth = textRenderer->Measure(m_NotificationText, fontSize);
    const float boxWidth = textWidth + (UiMetrics::kNotificationTextPaddingX * 2.0f);
    const float boxHeight = UiMetrics::kNotificationHeight;

    const float x = m_ScreenWidth - boxWidth - UiMetrics::kNotificationMargin;
    const float y = m_ScreenHeight - boxHeight - UiMetrics::kNotificationMargin;

    const Color boxColor = Fade(NookCol::UI_SHELL, alpha * 0.96f);
    const Color textColor = Fade(NookCol::UI_TEXT, alpha);

    DrawRectangleRounded({ x, y, boxWidth, boxHeight }, 0.3f, 10, boxColor);
    textRenderer->DrawSimpleText(m_NotificationText, { x + UiMetrics::kNotificationTextPaddingX, y + UiMetrics::kNotificationTextPaddingY }, fontSize, textColor);
}

bool UIManager::IsMouseOverUI() const {
    const Vector2 mouse = GetMousePosition();
    const Rectangle toolbarRect = {
        UiMetrics::kToolbarShellMargin,
        UiMetrics::kToolbarShellMargin,
        (float)m_ScreenWidth - (UiMetrics::kToolbarShellMargin * 2.0f),
        UiMetrics::kToolbarShellHeight
    };

    if (CheckCollisionPointRec(mouse, toolbarRect)) {
        return true;
    }

    for (const auto& w : m_Widgets) {
        if (!w->IsVisible()) {
            continue;
        }

        if (w->IsHovered() || CheckCollisionPointRec(mouse, w->GetBounds())) {
            return true;
        }
    }

    if (m_NodeContextMenuVisible && CheckCollisionPointRec(mouse, m_NodeContextMenuBounds)) {
        return true;
    }

    return false;
}
