#include "calendarWidget.h"

#include "../colors.h"
#include "../textRenderer.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <ctime>

namespace {
constexpr float kOuterPadding = 12.0f;
constexpr float kHeaderHeight = 36.0f;
constexpr float kWeekHeaderHeight = 22.0f;
constexpr float kGridGap = 4.0f;
constexpr float kCellHeight = 28.0f;

const std::array<const char*, 7> kWeekLabels = { "Mo", "Tu", "We", "Th", "Fr", "Sa", "Su" };
const std::array<const char*, 12> kMonthNames = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};
}

CalendarWidget::CalendarWidget(Anchor anchor, Vector2 offset, Vector2 size)
    : Widget(anchor, offset, size)
{
    isVisible = false;
    OnWindowResize(GetScreenWidth(), GetScreenHeight());
}

void CalendarWidget::Open(const std::string& initialDate, const std::function<void(const std::string&)>& onDateSelected, Vector2 preferredTopLeft)
{
    m_OnDateSelected = onDateSelected;

    int day = 0;
    int month = 0;
    int year = 0;
    if (TryParseDate(initialDate, day, month, year)) {
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
        m_Month = localTm.tm_mon + 1;
        m_Year = localTm.tm_year + 1900;
    }

    UpdateBoundsFromTopLeft(preferredTopLeft);
    isVisible = true;
}

void CalendarWidget::Close()
{
    isVisible = false;
    m_OnDateSelected = nullptr;
}

void CalendarWidget::Update()
{
    if (!isVisible) {
        isHovered = false;
        return;
    }

    const Vector2 mouse = GetMousePosition();
    isHovered = CheckCollisionPointRec(mouse, m_Bounds);

    if (isHovered) {
        Widget::DesiredCursor = MOUSE_CURSOR_POINTING_HAND;
    }

    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        return;
    }

    // Close on outside click.
    if (!isHovered) {
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

    const float gridTop = m_Bounds.y + kOuterPadding + kHeaderHeight + kWeekHeaderHeight + 6.0f;
    const float usableWidth = m_Bounds.width - (kOuterPadding * 2.0f);
    const float cellWidth = (usableWidth - (kGridGap * 6.0f)) / 7.0f;

    const int firstWeekday = WeekdayOfFirstDay(m_Year, m_Month); // Monday=0..Sunday=6
    const int totalDays = DaysInMonth(m_Year, m_Month);

    int dayCounter = 1;
    for (int row = 0; row < 6 && dayCounter <= totalDays; ++row) {
        for (int col = 0; col < 7 && dayCounter <= totalDays; ++col) {
            const int cellIndex = row * 7 + col;
            if (cellIndex < firstWeekday) {
                continue;
            }

            Rectangle cell{
                m_Bounds.x + kOuterPadding + col * (cellWidth + kGridGap),
                gridTop + row * (kCellHeight + kGridGap),
                cellWidth,
                kCellHeight
            };

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
    if (!isVisible || !renderer) {
        return;
    }

    DrawRectangleRounded(m_Bounds, 0.08f, 10, Fade(NookCol::UI_SHELL, 0.98f));
    DrawRectangleRoundedLinesEx(m_Bounds, 0.08f, 10, 2.0f, NookCol::UI_BORDER);

    const Rectangle headerRect{
        m_Bounds.x + kOuterPadding,
        m_Bounds.y + kOuterPadding,
        m_Bounds.width - 2.0f * kOuterPadding,
        kHeaderHeight
    };

    m_PrevMonthButtonBounds = {
        headerRect.x,
        headerRect.y + 4.0f,
        28.0f,
        headerRect.height - 8.0f
    };
    m_NextMonthButtonBounds = {
        headerRect.x + headerRect.width - 28.0f,
        headerRect.y + 4.0f,
        28.0f,
        headerRect.height - 8.0f
    };

    DrawRectangleRounded(m_PrevMonthButtonBounds, 0.2f, 8, NookCol::UI_PANEL);
    DrawRectangleRounded(m_NextMonthButtonBounds, 0.2f, 8, NookCol::UI_PANEL);

    renderer->DrawTextCentered("<", { m_PrevMonthButtonBounds.x + m_PrevMonthButtonBounds.width * 0.5f, m_PrevMonthButtonBounds.y + m_PrevMonthButtonBounds.height * 0.5f }, 18.0f, NookCol::UI_TEXT);
    renderer->DrawTextCentered(">", { m_NextMonthButtonBounds.x + m_NextMonthButtonBounds.width * 0.5f, m_NextMonthButtonBounds.y + m_NextMonthButtonBounds.height * 0.5f }, 18.0f, NookCol::UI_TEXT);

    std::string title = std::string(kMonthNames[(size_t)(m_Month - 1)]) + " " + std::to_string(m_Year);
    renderer->DrawTextCentered(title, { headerRect.x + headerRect.width * 0.5f, headerRect.y + headerRect.height * 0.5f }, 20.0f, NookCol::UI_TEXT);

    const float usableWidth = m_Bounds.width - (kOuterPadding * 2.0f);
    const float cellWidth = (usableWidth - (kGridGap * 6.0f)) / 7.0f;
    const float weekTop = m_Bounds.y + kOuterPadding + kHeaderHeight;

    for (int i = 0; i < 7; ++i) {
        const float x = m_Bounds.x + kOuterPadding + i * (cellWidth + kGridGap);
        renderer->DrawTextCentered(kWeekLabels[(size_t)i], { x + cellWidth * 0.5f, weekTop + kWeekHeaderHeight * 0.5f }, 16.0f, NookCol::UI_TEXT_MUTED);
    }

    const float gridTop = weekTop + kWeekHeaderHeight + 6.0f;
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
    for (int row = 0; row < 6 && dayCounter <= totalDays; ++row) {
        for (int col = 0; col < 7 && dayCounter <= totalDays; ++col) {
            const int cellIndex = row * 7 + col;
            if (cellIndex < firstWeekday) {
                continue;
            }

            Rectangle cell{
                m_Bounds.x + kOuterPadding + col * (cellWidth + kGridGap),
                gridTop + row * (kCellHeight + kGridGap),
                cellWidth,
                kCellHeight
            };

            const bool isToday = (dayCounter == todayDay && m_Month == todayMonth && m_Year == todayYear);
            const bool hovered = CheckCollisionPointRec(GetMousePosition(), cell);

            if (isToday) {
                DrawRectangleRounded(cell, 0.18f, 6, Fade(NookCol::UI_ACCENT, 0.28f));
                DrawRectangleRoundedLinesEx(cell, 0.18f, 6, 1.5f, NookCol::UI_ACCENT);
            }
            else if (hovered) {
                DrawRectangleRounded(cell, 0.18f, 6, NookCol::UI_PANEL_HOVER);
            }
            else {
                DrawRectangleRounded(cell, 0.18f, 6, NookCol::UI_PANEL_ALT);
            }

            renderer->DrawTextCentered(std::to_string(dayCounter), { cell.x + cell.width * 0.5f, cell.y + cell.height * 0.5f }, 16.0f, NookCol::UI_TEXT);
            ++dayCounter;
        }
    }
}

bool CalendarWidget::IsLeapYear(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int CalendarWidget::DaysInMonth(int year, int month)
{
    static const int kDaysByMonth[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    const int base = kDaysByMonth[month - 1];
    return (month == 2 && IsLeapYear(year)) ? 29 : base;
}

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
    std::snprintf(buffer, sizeof(buffer), "%02d.%02d.%04d", day, month, year);
    return std::string(buffer);
}

bool CalendarWidget::TryParseDate(const std::string& text, int& outDay, int& outMonth, int& outYear)
{
    if (text.size() != 10 || text[2] != '.' || text[5] != '.') {
        return false;
    }

    for (size_t i = 0; i < text.size(); ++i) {
        if (i == 2 || i == 5) {
            continue;
        }
        if (text[i] < '0' || text[i] > '9') {
            return false;
        }
    }

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
    topLeft.x = std::clamp(topLeft.x, 8.0f, std::max(8.0f, (float)GetScreenWidth() - m_Size.x - 8.0f));
    topLeft.y = std::clamp(topLeft.y, 8.0f, std::max(8.0f, (float)GetScreenHeight() - m_Size.y - 8.0f));

    SetOffset({ topLeft.x + m_Size.x * 0.5f, topLeft.y + m_Size.y * 0.5f });
    OnWindowResize(GetScreenWidth(), GetScreenHeight());
}
