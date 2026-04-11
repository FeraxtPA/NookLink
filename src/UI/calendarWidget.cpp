
// Implementation of the CalendarWidget class.
// Renders interactive calendar with navigation and date selection.


#include "calendarWidget.h"

#include "../colors.h"
#include "../constants.h"
#include "../textRenderer.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <ctime>

namespace {
const std::array<const char*, 7> kWeekLabels = { "Mo", "Tu", "We", "Th", "Fr", "Sa", "Su" };
const std::array<const char*, 12> kMonthNames = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};
}

CalendarWidget::CalendarWidget(Anchor anchor, Vector2 offset, Vector2 size)
    : Widget(anchor, offset, size)
{
    m_IsVisible = false;
    OnWindowResize(GetScreenWidth(), GetScreenHeight());
}

void CalendarWidget::Open(const std::string& initialDate, const std::function<void(const std::string&)>& onDateSelected, Vector2 preferredTopLeft)
{
    m_OnDateSelected = onDateSelected;

    int day = 0;
    int month = 0;
    int year = 0;
    if (TryParseDate(initialDate, day, month, year)) {
        // Start from input date when available so picker opens in expected month.
        m_Month = month;
        m_Year = year;
    }
    else {
        const std::time_t now = std::time(nullptr);
        std::tm localTm{};
#if defined(_WIN32)
        localtime_s(&localTm, &now);
#else
        localtime_r(&now, &localTm);
#endif
		// Fall back to current month/year when input is missing or invalid.
        m_Month = localTm.tm_mon + 1;
        m_Year = localTm.tm_year + 1900;
    }

    UpdateBoundsFromTopLeft(preferredTopLeft);
    m_IsVisible = true;
}

void CalendarWidget::Close()
{
    m_IsVisible = false;
    m_OnDateSelected = nullptr;
}

void CalendarWidget::Update()
{
    if (!m_IsVisible) {
        m_IsHovered = false;
        return;
    }

    const Vector2 mouse = GetMousePosition();
    m_IsHovered = CheckCollisionPointRec(mouse, m_Bounds);

    if (m_IsHovered) {
		RequestCursor(MOUSE_CURSOR_POINTING_HAND);
    }

    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        return;
    }

    // Close on outside click.
    if (!m_IsHovered) {
        Close();
        return;
    }

    if (CheckCollisionPointRec(mouse, m_PrevMonthButtonBounds)) {
        StepMonth(-1);
        return;
    }

    if (CheckCollisionPointRec(mouse, m_NextMonthButtonBounds)) {
        StepMonth(1);
        return;
    }

    // Compute grid geometry once; reused for all hit-tests below.
    const float gridTop = m_Bounds.y + NookConst::Calendar::kOuterPadding + NookConst::Calendar::kHeaderHeight + NookConst::Calendar::kWeekHeaderHeight + NookConst::Calendar::kGridTopGap;
    const float usableWidth = m_Bounds.width - (NookConst::Calendar::kOuterPadding * 2.0f);
    const float cellWidth = (usableWidth - (NookConst::Calendar::kGridGap * 6.0f)) / 7.0f;

    const int firstWeekday = WeekdayOfFirstDay(m_Year, m_Month); // Monday=0..Sunday=6
    const int totalDays = DaysInMonth(m_Year, m_Month);

    int dayCounter = 1;
    // Walk a fixed 6x7 grid and skip leading cells before weekday-of-1st.
    for (int row = 0; row < 6 && dayCounter <= totalDays; ++row) {
        for (int col = 0; col < 7 && dayCounter <= totalDays; ++col) {
            const int cellIndex = row * 7 + col;

			// Skip cells before the first day of the month. These are always present to keep the grid layout consistent, but are not interactive.
            if (cellIndex < firstWeekday) {
                continue;
            }

            Rectangle cell{
                m_Bounds.x + NookConst::Calendar::kOuterPadding + col * (cellWidth + NookConst::Calendar::kGridGap),
                gridTop + row * (NookConst::Calendar::kCellHeight + NookConst::Calendar::kGridGap),
                cellWidth,
                NookConst::Calendar::kCellHeight
            };

			// Check if this cell was clicked. If so, invoke callback with selected date and close picker.
            if (CheckCollisionPointRec(mouse, cell)) {
                if (m_OnDateSelected) {
                    m_OnDateSelected(FormatDate(dayCounter, m_Month, m_Year));
                }
                Close();
                return;
            }

            ++dayCounter;
        }
    }
}

void CalendarWidget::Draw(TextRenderer* renderer)
{
    if (!m_IsVisible || !renderer) {
        return;
    }

	// Background and border.
    DrawRectangleRounded(m_Bounds, NookConst::Calendar::kBackgroundRoundness, NookConst::Calendar::kBackgroundRoundSegments, Fade(NookCol::UI_SHELL, 0.98f));
    DrawRectangleRoundedLinesEx(m_Bounds, NookConst::Calendar::kBackgroundRoundness, NookConst::Calendar::kBackgroundRoundSegments, NookConst::Calendar::kBorderThickness, NookCol::UI_BORDER);

	// Header with month/year and navigation buttons.
    const Rectangle headerRect{
        m_Bounds.x + NookConst::Calendar::kOuterPadding,
        m_Bounds.y + NookConst::Calendar::kOuterPadding,
        m_Bounds.width - 2.0f * NookConst::Calendar::kOuterPadding,
        NookConst::Calendar::kHeaderHeight
    };

    
    m_PrevMonthButtonBounds = {
        headerRect.x,
        headerRect.y + NookConst::Calendar::kNavButtonTopInset,
        NookConst::Calendar::kNavButtonWidth,
        headerRect.height - NookConst::Calendar::kNavButtonVerticalInset
    };
    m_NextMonthButtonBounds = {
        headerRect.x + headerRect.width - NookConst::Calendar::kNavButtonWidth,
        headerRect.y + NookConst::Calendar::kNavButtonTopInset,
        NookConst::Calendar::kNavButtonWidth,
        headerRect.height - NookConst::Calendar::kNavButtonVerticalInset
    };

	// Draw month navigation buttons and month/year title.
    DrawRectangleRounded(m_PrevMonthButtonBounds, NookConst::Calendar::kNavButtonRoundness, NookConst::Calendar::kNavButtonRoundSegments, NookCol::UI_PANEL);
    DrawRectangleRounded(m_NextMonthButtonBounds, NookConst::Calendar::kNavButtonRoundness, NookConst::Calendar::kNavButtonRoundSegments, NookCol::UI_PANEL);

    renderer->DrawTextCentered("<", { m_PrevMonthButtonBounds.x + m_PrevMonthButtonBounds.width * 0.5f, m_PrevMonthButtonBounds.y + m_PrevMonthButtonBounds.height * 0.5f }, NookConst::Calendar::kNavButtonFontSize, NookCol::UI_TEXT);
    renderer->DrawTextCentered(">", { m_NextMonthButtonBounds.x + m_NextMonthButtonBounds.width * 0.5f, m_NextMonthButtonBounds.y + m_NextMonthButtonBounds.height * 0.5f }, NookConst::Calendar::kNavButtonFontSize, NookCol::UI_TEXT);

	// Center month/year title between nav buttons.
    std::string title = std::string(kMonthNames[(size_t)(m_Month - 1)]) + " " + std::to_string(m_Year);
    renderer->DrawTextCentered(title, { headerRect.x + headerRect.width * 0.5f, headerRect.y + headerRect.height * 0.5f }, NookConst::Calendar::kTitleFontSize, NookCol::UI_TEXT);

    // Cell width adapts to widget width so the 7-day grid always fits exactly.
    const float usableWidth = m_Bounds.width - (NookConst::Calendar::kOuterPadding * 2.0f);
    const float cellWidth = (usableWidth - (NookConst::Calendar::kGridGap * 6.0f)) / 7.0f;
    const float weekTop = m_Bounds.y + NookConst::Calendar::kOuterPadding + NookConst::Calendar::kHeaderHeight;

	// Draw weekday labels above the grid.
    for (int i = 0; i < 7; ++i) {
        const float x = m_Bounds.x + NookConst::Calendar::kOuterPadding + i * (cellWidth + NookConst::Calendar::kGridGap);
        renderer->DrawTextCentered(kWeekLabels[(size_t)i], { x + cellWidth * 0.5f, weekTop + NookConst::Calendar::kWeekHeaderHeight * 0.5f }, NookConst::Calendar::kWeekdayFontSize, NookCol::UI_TEXT_MUTED);
    }

    const float gridTop = weekTop + NookConst::Calendar::kWeekHeaderHeight + NookConst::Calendar::kGridTopGap;
    const int firstWeekday = WeekdayOfFirstDay(m_Year, m_Month);
    const int totalDays = DaysInMonth(m_Year, m_Month);

    const std::time_t now = std::time(nullptr);
    std::tm localTm{};
#if defined(_WIN32)
    localtime_s(&localTm, &now);
#else
    localtime_r(&now, &localTm);
#endif
    const int todayDay = localTm.tm_mday;
    const int todayMonth = localTm.tm_mon + 1;
    const int todayYear = localTm.tm_year + 1900;

    int dayCounter = 1;
    // Render days in row-major order with the same indexing scheme used by Update().
    for (int row = 0; row < 6 && dayCounter <= totalDays; ++row) {
        for (int col = 0; col < 7 && dayCounter <= totalDays; ++col) {
            const int cellIndex = row * 7 + col;
            if (cellIndex < firstWeekday) {
                continue;
            }

            Rectangle cell{
                m_Bounds.x + NookConst::Calendar::kOuterPadding + col * (cellWidth + NookConst::Calendar::kGridGap),
                gridTop + row * (NookConst::Calendar::kCellHeight + NookConst::Calendar::kGridGap),
                cellWidth,
                NookConst::Calendar::kCellHeight
            };

            const bool isToday = (dayCounter == todayDay && m_Month == todayMonth && m_Year == todayYear);
            const bool hovered = CheckCollisionPointRec(GetMousePosition(), cell);

            if (isToday) {
                DrawRectangleRounded(cell, NookConst::Calendar::kDayCellRoundness, NookConst::Calendar::kDayCellRoundSegments, Fade(NookCol::UI_ACCENT, 0.28f));
                DrawRectangleRoundedLinesEx(cell, NookConst::Calendar::kDayCellRoundness, NookConst::Calendar::kDayCellRoundSegments, NookConst::Calendar::kTodayBorderThickness, NookCol::UI_ACCENT);
            }
            else if (hovered) {
                DrawRectangleRounded(cell, NookConst::Calendar::kDayCellRoundness, NookConst::Calendar::kDayCellRoundSegments, NookCol::UI_PANEL_HOVER);
            }
            else {
                DrawRectangleRounded(cell, NookConst::Calendar::kDayCellRoundness, NookConst::Calendar::kDayCellRoundSegments, NookCol::UI_PANEL_ALT);
            }
			// Draw day number centered in cell.
            renderer->DrawTextCentered(std::to_string(dayCounter), { cell.x + cell.width * 0.5f, cell.y + cell.height * 0.5f }, NookConst::Calendar::kDayFontSize, NookCol::UI_TEXT);
            ++dayCounter;
        }
    }
}

bool CalendarWidget::IsLeapYear(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// Return number of days in the given month/year, accounting for leap years in February.
int CalendarWidget::DaysInMonth(int year, int month)
{
    static const int kDaysByMonth[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    const int base = kDaysByMonth[month - 1];
    return (month == 2 && IsLeapYear(year)) ? 29 : base;
}

// Compute the weekday of the first day of the given month/year using std::tm and mktime.
int CalendarWidget::WeekdayOfFirstDay(int year, int month)
{
    std::tm first{};
    first.tm_mday = 1;
    first.tm_mon = month - 1;
    first.tm_year = year - 1900;
    first.tm_isdst = -1;

    
    std::mktime(&first);

    // tm_wday: Sunday=0 ... Saturday=6 -> Monday=0 ... Sunday=6
    return (first.tm_wday + 6) % 7;
}

std::string CalendarWidget::FormatDate(int day, int month, int year)
{
    char buffer[16]{};
	// Format as "DD.MM.YYYY" with leading zeros for day and month.
    std::snprintf(buffer, sizeof(buffer), "%02d.%02d.%04d", day, month, year);
    return std::string(buffer);
}

bool CalendarWidget::TryParseDate(const std::string& text, int& outDay, int& outMonth, int& outYear)
{
	// Expect strict "DD.MM.YYYY" format to avoid ambiguity and simplify parsing.
    if (text.size() != 10 || text[2] != '.' || text[5] != '.') {
        return false;
    }

	// Validate that all other characters are digits.
    for (size_t i = 0; i < text.size(); ++i) {
        if (i == 2 || i == 5) {
            continue;
        }
        if (text[i] < '0' || text[i] > '9') {
            return false;
        }
    }
	// Extract numeric values after validation.
    outDay = std::stoi(text.substr(0, 2));
    outMonth = std::stoi(text.substr(3, 2));
    outYear = std::stoi(text.substr(6, 4));

    
    if (outYear < 1900 || outYear > 3000 || outMonth < 1 || outMonth > 12) {
        return false;
    }

    const int maxDay = DaysInMonth(outYear, outMonth);

    return outDay >= 1 && outDay <= maxDay;
}

void CalendarWidget::StepMonth(int delta)
{
    m_Month += delta;
    // Normalize month into [1, 12] while carrying year across boundaries.
    while (m_Month < 1) {
        m_Month += 12;
        --m_Year;
    }
    while (m_Month > 12) {
        m_Month -= 12;
        ++m_Year;
    }
}

void CalendarWidget::UpdateBoundsFromTopLeft(Vector2 topLeft)
{
    // Keep popup fully visible on screen.
    topLeft.x = std::clamp(topLeft.x, NookConst::Calendar::kScreenPadding, std::max(NookConst::Calendar::kScreenPadding, (float)GetScreenWidth() - m_Size.x - NookConst::Calendar::kScreenPadding));
    topLeft.y = std::clamp(topLeft.y, NookConst::Calendar::kScreenPadding, std::max(NookConst::Calendar::kScreenPadding, (float)GetScreenHeight() - m_Size.y - NookConst::Calendar::kScreenPadding));

    SetOffset({ topLeft.x + m_Size.x * 0.5f, topLeft.y + m_Size.y * 0.5f });
    OnWindowResize(GetScreenWidth(), GetScreenHeight());
}
