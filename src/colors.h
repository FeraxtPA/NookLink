
// Color palette and theme definitions for the NookLink UI.
// Defines colors for nodes, UI elements, text, and status indicators.


#pragma once
#include <raylib.h>

namespace NookCol
{
	enum class ThemePreset {
		Classic,
		KaerMorhen,
		BlueHour,
		Autumn,
		Frosted
	};

	extern Color BACKGROUND;
	extern Color EDGE;
	extern Color CURRENTLY_READING;
	extern Color TO_READ;
	extern Color READ;
	extern Color GENRE;
	extern Color POPUP_BG;
	extern Color POPUP_BORDER;
	extern Color TEXT_DEFAULT;
	extern Color TEXT_HIGHLIGHT;
	extern Color TEXT_ONNODE;

	extern Color UI_SHELL;
	extern Color UI_PANEL;
	extern Color UI_PANEL_ALT;
	extern Color UI_PANEL_HOVER;
	extern Color UI_BORDER;
	extern Color UI_BORDER_SOFT;
	extern Color UI_TEXT;
	extern Color UI_TEXT_MUTED;
	extern Color UI_ACCENT;
	extern Color UI_ACCENT_SOFT;

	void ApplyThemePreset(ThemePreset preset);
	void ApplyThemePresetByIndex(int index);
	ThemePreset GetCurrentThemePreset();
	int GetCurrentThemeIndex();
	int GetThemeCount();
	const char* GetCurrentThemeName();
	const char* GetThemeNameByIndex(int index);
}