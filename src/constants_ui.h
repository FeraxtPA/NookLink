#pragma once

#include <raylib.h>

namespace NookConst {

namespace UI {
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

inline constexpr Vector2 kEditorPanelSize = { 460.0f, 760.0f };
inline constexpr Vector2 kEditorPanelOffset = { 0.0f, 0.0f };

inline constexpr Vector2 kLotteryPanelSize = { 400.0f, 350.0f };
inline constexpr Vector2 kLotteryPanelOffset = { 0.0f, 25.0f };
inline constexpr float kLotteryTextHeight = 180.0f;

inline constexpr Vector2 kGoalPanelSize = { 380.0f, 360.0f };
inline constexpr Vector2 kGoalPanelOffset = { 0.0f, 0.0f };
inline constexpr float kGoalButtonWidth = 80.0f;

inline constexpr Vector2 kAnalyticsPanelOffset = { 0.0f, 0.0f };

inline constexpr Vector2 kSettingsPanelSize = { 820.0f, 720.0f };
inline constexpr Vector2 kSettingsPanelOffset = { 0.0f, 10.0f };
inline constexpr float kSettingsControlWidth = 740.0f;

inline constexpr int kDefaultFontSize = 20;
inline constexpr float kNotificationMargin = 20.0f;
inline constexpr float kNotificationHeight = 50.0f;
inline constexpr float kNotificationTextPaddingX = 20.0f;
inline constexpr float kNotificationTextPaddingY = 15.0f;

} // namespace UI

namespace Text {
inline constexpr const char* kFilterToolbarGlyph = "☰";
inline constexpr const char* kFilterToolbarGlyphActive = "☰*";
inline constexpr const char* kSettingsToolbarGlyph = "⚙";
inline constexpr const char* kDetailsDivider = "--------------------------------";
} // namespace Text

namespace WidgetStyle {
inline constexpr float kButtonRoundness = 0.22f;
inline constexpr int kButtonRoundSegments = 10;
inline constexpr float kButtonBorderThickness = 2.0f;
inline constexpr float kButtonFontSize = 19.0f;

inline constexpr float kCheckboxRoundness = 0.18f;
inline constexpr int kCheckboxRoundSegments = 8;
inline constexpr float kCheckboxBorderThickness = 2.0f;
inline constexpr float kCheckboxInnerInset = 4.0f;
inline constexpr float kCheckboxLabelGap = 10.0f;
inline constexpr float kCheckboxLabelFontSize = 20.0f;
inline constexpr float kCheckboxClickLabelExtension = 200.0f;

inline constexpr float kSliderRoundness = 0.5f;
inline constexpr int kSliderRoundSegments = 12;
inline constexpr float kSliderBorderThickness = 2.0f;
inline constexpr float kSliderStepScale = 10.0f;

inline constexpr float kDropdownBaseRoundness = 0.2f;
inline constexpr int kDropdownBaseRoundSegments = 8;
inline constexpr float kDropdownBorderThickness = 2.0f;
inline constexpr float kDropdownOptionBorderThickness = 1.0f;
inline constexpr float kDropdownTextPaddingX = 12.0f;
inline constexpr float kDropdownTextPaddingY = 9.0f;
inline constexpr float kDropdownArrowRightInset = 22.0f;
inline constexpr float kDropdownArrowTopInset = 8.0f;
inline constexpr float kDropdownFontSize = 20.0f;
inline constexpr float kDropdownScrollbarTrackWidth = 6.0f;
inline constexpr float kDropdownScrollbarTrackPadding = 4.0f;
inline constexpr float kDropdownScrollbarThumbMinHeight = 16.0f;
inline constexpr float kDropdownScrollbarRoundness = 0.6f;
inline constexpr int kDropdownScrollbarRoundSegments = 6;
} // namespace WidgetStyle

namespace Calendar {
inline constexpr float kOuterPadding = 12.0f;
inline constexpr float kHeaderHeight = 36.0f;
inline constexpr float kWeekHeaderHeight = 22.0f;
inline constexpr float kGridGap = 4.0f;
inline constexpr float kCellHeight = 28.0f;
inline constexpr float kGridTopGap = 6.0f;
inline constexpr float kNavButtonTopInset = 4.0f;
inline constexpr float kNavButtonWidth = 28.0f;
inline constexpr float kNavButtonVerticalInset = 8.0f;
inline constexpr float kBackgroundRoundness = 0.08f;
inline constexpr int kBackgroundRoundSegments = 10;
inline constexpr float kBorderThickness = 2.0f;
inline constexpr float kNavButtonRoundness = 0.2f;
inline constexpr int kNavButtonRoundSegments = 8;
inline constexpr float kNavButtonFontSize = 18.0f;
inline constexpr float kTitleFontSize = 20.0f;
inline constexpr float kWeekdayFontSize = 16.0f;
inline constexpr float kDayCellRoundness = 0.18f;
inline constexpr int kDayCellRoundSegments = 6;
inline constexpr float kTodayBorderThickness = 1.5f;
inline constexpr float kDayFontSize = 16.0f;
inline constexpr float kScreenPadding = 8.0f;
} // namespace Calendar

} // namespace NookConst
