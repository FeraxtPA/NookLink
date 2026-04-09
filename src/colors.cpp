// Runtime theme palette implementation.
// Exposes mutable global colors and preset switching helpers.

#include "colors.h"

#include <array>
#include <algorithm>

namespace NookCol {
namespace {
struct ThemePalette {
    const char* name;
    Color background;
    Color edge;
    Color currentlyReading;
    Color toRead;
    Color read;
    Color genre;
    Color popupBg;
    Color popupBorder;
    Color textDefault;
    Color textHighlight;
    Color textOnNode;
    Color uiShell;
    Color uiPanel;
    Color uiPanelAlt;
    Color uiPanelHover;
    Color uiBorder;
    Color uiBorderSoft;
    Color uiText;
    Color uiTextMuted;
    Color uiAccent;
    Color uiAccentSoft;
};

constexpr std::array<ThemePalette, 5> kThemes = {{
    {
        "Classic",
        {60, 58, 56, 255},
        {80, 78, 76, 255},
        {190, 70, 60, 255},
        {200, 150, 50, 255},
        {140, 130, 40, 255},
        {80, 130, 135, 255},
        {70, 68, 66, 255},
        {200, 150, 50, 255},
        {235, 219, 178, 255},
        {120, 150, 150, 255},
        {235, 219, 178, 255},
        {47, 43, 40, 232},
        {59, 54, 50, 236},
        {66, 60, 55, 236},
        {78, 71, 65, 240},
        {113, 103, 93, 255},
        {89, 82, 75, 255},
        {236, 225, 208, 255},
        {188, 174, 160, 255},
        {214, 177, 112, 255},
        {164, 187, 151, 255},
    },
    {
        "Kaer Morhen",
        {40, 44, 48, 255},
        {95, 95, 95, 255},
        {162, 55, 60, 255},
        {210, 170, 80, 255},
        {108, 88, 68, 255},
        {85, 115, 80, 255},
        {52, 56, 60, 255},
        {120, 105, 85, 255},
        {220, 220, 220, 255},
        {60, 160, 160, 255},
        {235, 235, 235, 255},
        {34, 38, 42, 232},
        {48, 52, 56, 236},
        {55, 60, 64, 236},
        {66, 72, 76, 240},
        {90, 96, 100, 255},
        {74, 80, 84, 255},
        {230, 231, 233, 255},
        {172, 177, 181, 255},
        {176, 156, 119, 255},
        {126, 166, 153, 255},
    },
    {
        "Blue Hour",
        {245, 248, 245, 255},
        {192, 200, 180, 255},
        {212, 143, 155, 255},
        {200, 222, 167, 255},
        {162, 173, 150, 255},
        {120, 132, 105, 255},
        {255, 255, 255, 255},
        {220, 225, 210, 255},
        {45, 55, 40, 255},
        {102, 122, 80, 255},
        {65, 75, 55, 255},
        {231, 237, 229, 245},
        {239, 244, 236, 245},
        {228, 236, 224, 245},
        {216, 227, 211, 245},
        {177, 191, 168, 255},
        {199, 210, 189, 255},
        {61, 73, 56, 255},
        {97, 111, 89, 255},
        {148, 170, 116, 255},
        {126, 156, 145, 255},
    },
    {
        "Autumn",
        {250, 245, 235, 255},
        {190, 175, 160, 255},
        {210, 116, 86, 255},
        {234, 189, 93, 255},
        {166, 123, 91, 255},
        {124, 94, 75, 255},
        {255, 253, 245, 255},
        {215, 204, 180, 255},
        {55, 40, 30, 255},
        {198, 108, 58, 255},
        {70, 50, 40, 255},
        {234, 224, 206, 245},
        {245, 235, 220, 245},
        {236, 225, 210, 245},
        {225, 213, 196, 245},
        {194, 166, 141, 255},
        {206, 181, 158, 255},
        {78, 57, 42, 255},
        {126, 101, 83, 255},
        {198, 139, 84, 255},
        {146, 154, 114, 255},
    },
    {
        "Frosted",
        {250, 250, 255, 255},
        {210, 210, 220, 255},
        {243, 180, 190, 255},
        {255, 245, 195, 255},
        {202, 195, 230, 255},
        {190, 210, 225, 255},
        {255, 255, 255, 255},
        {220, 220, 230, 255},
        {50, 60, 85, 255},
        {130, 160, 220, 255},
        {70, 70, 90, 255},
        {235, 239, 249, 245},
        {244, 247, 253, 245},
        {235, 241, 250, 245},
        {226, 233, 245, 245},
        {188, 201, 225, 255},
        {204, 214, 234, 255},
        {60, 72, 98, 255},
        {114, 128, 154, 255},
        {122, 159, 217, 255},
        {146, 178, 194, 255},
    },
}};

ThemePreset g_currentTheme = ThemePreset::Classic;

void SetFromPalette(const ThemePalette& theme) {
    BACKGROUND = theme.background;
    EDGE = theme.edge;
    CURRENTLY_READING = theme.currentlyReading;
    TO_READ = theme.toRead;
    READ = theme.read;
    GENRE = theme.genre;
    POPUP_BG = theme.popupBg;
    POPUP_BORDER = theme.popupBorder;
    TEXT_DEFAULT = theme.textDefault;
    TEXT_HIGHLIGHT = theme.textHighlight;
    TEXT_ONNODE = theme.textOnNode;

    UI_SHELL = theme.uiShell;
    UI_PANEL = theme.uiPanel;
    UI_PANEL_ALT = theme.uiPanelAlt;
    UI_PANEL_HOVER = theme.uiPanelHover;
    UI_BORDER = theme.uiBorder;
    UI_BORDER_SOFT = theme.uiBorderSoft;
    UI_TEXT = theme.uiText;
    UI_TEXT_MUTED = theme.uiTextMuted;
    UI_ACCENT = theme.uiAccent;
    UI_ACCENT_SOFT = theme.uiAccentSoft;
}
} // namespace

Color BACKGROUND = {60, 58, 56, 255};
Color EDGE = {80, 78, 76, 255};
Color CURRENTLY_READING = {190, 70, 60, 255};
Color TO_READ = {200, 150, 50, 255};
Color READ = {140, 130, 40, 255};
Color GENRE = {80, 130, 135, 255};
Color POPUP_BG = {70, 68, 66, 255};
Color POPUP_BORDER = {200, 150, 50, 255};
Color TEXT_DEFAULT = {235, 219, 178, 255};
Color TEXT_HIGHLIGHT = {120, 150, 150, 255};
Color TEXT_ONNODE = {235, 219, 178, 255};

Color UI_SHELL = {47, 43, 40, 232};
Color UI_PANEL = {59, 54, 50, 236};
Color UI_PANEL_ALT = {66, 60, 55, 236};
Color UI_PANEL_HOVER = {78, 71, 65, 240};
Color UI_BORDER = {113, 103, 93, 255};
Color UI_BORDER_SOFT = {89, 82, 75, 255};
Color UI_TEXT = {236, 225, 208, 255};
Color UI_TEXT_MUTED = {188, 174, 160, 255};
Color UI_ACCENT = {214, 177, 112, 255};
Color UI_ACCENT_SOFT = {164, 187, 151, 255};

void ApplyThemePreset(ThemePreset preset) {
    const int index = std::clamp((int)preset, 0, (int)kThemes.size() - 1);
    g_currentTheme = static_cast<ThemePreset>(index);
    SetFromPalette(kThemes[(size_t)index]);
}

void ApplyThemePresetByIndex(int index) {
    const int normalized = std::clamp(index, 0, (int)kThemes.size() - 1);
    ApplyThemePreset(static_cast<ThemePreset>(normalized));
}

ThemePreset GetCurrentThemePreset() {
    return g_currentTheme;
}

int GetCurrentThemeIndex() {
    return (int)g_currentTheme;
}

int GetThemeCount() {
    return (int)kThemes.size();
}

const char* GetCurrentThemeName() {
    return kThemes[(size_t)GetCurrentThemeIndex()].name;
}

const char* GetThemeNameByIndex(int index) {
    const int normalized = std::clamp(index, 0, (int)kThemes.size() - 1);
    return kThemes[(size_t)normalized].name;
}

} // namespace NookCol
