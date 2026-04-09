#pragma once

#include "widget.h"

#include <functional>
#include <string>

class CalendarWidget : public Widget {
public:
    CalendarWidget(Anchor anchor, Vector2 offset, Vector2 size);

    void Update() override;
    void Draw(TextRenderer* renderer) override;

    void Open(const std::string& initialDate, const std::function<void(const std::string&)>& onDateSelected, Vector2 preferredTopLeft);
    void Close();
    bool IsOpen() const { return isVisible; }

private:
    std::function<void(const std::string&)> m_OnDateSelected;

    int m_Year = 1970;
    int m_Month = 1;

    Rectangle m_PrevMonthButtonBounds{};
    Rectangle m_NextMonthButtonBounds{};

    static bool IsLeapYear(int year);
    static int DaysInMonth(int year, int month);
    static int WeekdayOfFirstDay(int year, int month);

    static std::string FormatDate(int day, int month, int year);
    static bool TryParseDate(const std::string& text, int& outDay, int& outMonth, int& outYear);

    void StepMonth(int delta);
    void UpdateBoundsFromTopLeft(Vector2 topLeft);
};
