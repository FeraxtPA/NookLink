
// Internal metrics and constants for UI layout.
// Defines spacing, sizes, and positioning constants for UI components.


#pragma once

#include <raylib.h>

namespace UiMetrics {
inline constexpr float kToolbarShellMargin = 8.0f;
inline constexpr float kToolbarShellHeight = 98.0f;
inline constexpr float kToolbarRowTop = 54.0f;
inline constexpr float kToolbarRowHeight = 40.0f;
inline constexpr float kToolbarGap = 12.0f;
inline constexpr float kToolbarWidthInset = 52.0f;

inline constexpr float kToolbarBackWidth = 100.0f;
inline constexpr float kToolbarLoadWidth = 100.0f;
inline constexpr float kToolbarSaveWidth = 100.0f;
inline constexpr float kToolbarSaveAsWidth = 112.0f;
inline constexpr float kToolbarSearchWidth = 360.0f;
inline constexpr float kToolbarFiltersWidth = 58.0f;
inline constexpr float kToolbarSettingsWidth = 58.0f;
inline constexpr float kToolbarUndoWidth = 90.0f;
inline constexpr float kToolbarRedoWidth = 90.0f;
inline constexpr float kToolbarAddBookWidth = 110.0f;
inline constexpr float kToolbarNextReadWidth = 120.0f;
inline constexpr float kToolbarToggleLayoutWidth = 148.0f;
inline constexpr float kToolbarGoalsWidth = 88.0f;
inline constexpr float kToolbarStatsWidth = 88.0f;

inline constexpr float kToolbarLabelY = 18.0f;
inline constexpr float kToolbarLibraryX = 24.0f;
inline constexpr float kToolbarSearchHalfLabelWidth = 35.0f;
inline constexpr float kToolbarActionsInset = 172.0f;
inline constexpr int kToolbarLabelFontSize = 14;

inline constexpr float kPanelTitleBarHeight = 40.0f;
inline constexpr float kPanelGap = 12.0f;
inline constexpr float kPanelButtonRowHeight = 40.0f;
inline constexpr float kPanelInputHeight = 35.0f;
inline constexpr float kPanelWideFieldWidth = 360.0f;
inline constexpr float kPanelButtonRowWidth = 364.0f;

inline constexpr Vector2 kPanelPaddingCompact = { 18.0f, 16.0f };
inline constexpr Vector2 kPanelPaddingDefault = { 18.0f, 18.0f };
inline constexpr Vector2 kPanelPaddingDetails = { 16.0f, 16.0f };

inline constexpr Vector2 kFilterPanelSize = { 340.0f, 750.0f };
inline constexpr Vector2 kFilterPanelOffset = { -175.0f, 0.0f };
inline constexpr float kFilterControlWidth = 290.0f;
inline constexpr float kFilterCheckboxSize = 20.0f;

inline constexpr Vector2 kDetailsPanelSize = { 320.0f, 500.0f };
inline constexpr Vector2 kDetailsPanelOffset = { 180.0f, 0.0f };
inline constexpr float kDetailsTextWidth = 280.0f;
inline constexpr float kDetailsTextHeight = 320.0f;
inline constexpr float kDetailsButtonRowWidth = 288.0f;

inline constexpr Vector2 kEditorPanelSize = { 400.0f, 660.0f };
inline constexpr Vector2 kEditorPanelOffset = { 0.0f, 0.0f };

inline constexpr Vector2 kLotteryPanelSize = { 400.0f, 350.0f };
inline constexpr Vector2 kLotteryPanelOffset = { 0.0f, 25.0f };
inline constexpr float kLotteryTextHeight = 180.0f;

inline constexpr Vector2 kGoalPanelSize = { 380.0f, 360.0f };
inline constexpr Vector2 kGoalPanelOffset = { 0.0f, 0.0f };
inline constexpr float kGoalButtonWidth = 80.0f;

inline constexpr Vector2 kAnalyticsPanelSize = { 760.0f, 640.0f };
inline constexpr Vector2 kAnalyticsPanelOffset = { 0.0f, 0.0f };
inline constexpr float kAnalyticsLabelWidth = 680.0f;

inline constexpr Vector2 kSettingsPanelSize = { 820.0f, 620.0f };
inline constexpr Vector2 kSettingsPanelOffset = { 0.0f, 10.0f };
inline constexpr float kSettingsControlWidth = 740.0f;

inline constexpr float kHelpX = 10.0f;
inline constexpr float kHelpStartY = 136.0f;
inline constexpr float kHelpLineStep = 25.0f;

inline constexpr int kDefaultFontSize = 20;
inline constexpr float kNotificationMargin = 20.0f;
inline constexpr float kNotificationHeight = 50.0f;
inline constexpr float kNotificationTextPaddingX = 20.0f;
inline constexpr float kNotificationTextPaddingY = 15.0f;

inline constexpr float kTooltipOffset = 10.0f;
inline constexpr float kTooltipTextInset = 26.0f;
inline constexpr float kTooltipLineSpacing = 32.0f;
} // namespace UiMetrics
