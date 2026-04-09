// Simple dropdown widget for selecting a value from a list.
// Supports click-to-open options and callback on selection change.

#pragma once

#include "widget.h"

#include <functional>
#include <string>
#include <vector>

class Dropdown : public Widget {
public:
    Dropdown(
        Anchor anchor,
        Vector2 offset,
        Vector2 size,
        std::vector<std::string> options,
        int initialIndex = 0,
        std::function<void(int, const std::string&)> onSelect = nullptr);

    void Update() override;
    void Draw(TextRenderer* renderer) override;

    void SetOptions(const std::vector<std::string>& options, int selectedIndex = 0);
    void SetSelectedIndex(int index, bool notify = false);

    int GetSelectedIndex() const { return m_SelectedIndex; }
    std::string GetSelectedText() const;
    bool IsExpanded() const { return m_IsExpanded; }

private:
    std::vector<std::string> m_Options;
    int m_SelectedIndex = 0;
    bool m_IsExpanded = false;
    int m_HoveredOptionIndex = -1;
    std::function<void(int, const std::string&)> m_OnSelect;
};
