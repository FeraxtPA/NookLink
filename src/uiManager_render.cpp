
// UI rendering implementation.
// Handles drawing of all UI components and visual elements.


#include "uiManager.h"

#include "colors.h"
#include "uiManager_internal.h"

void UIManager::Draw(Vector2 mousePos, GraphManager* graphRenderer, const BookManager& bookManager, TextRenderer* textRenderer) const
{
    if (!textRenderer) return;

    const Rectangle toolbarRect = {
        UiMetrics::kToolbarShellMargin,
        UiMetrics::kToolbarShellMargin,
        (float)m_ScreenWidth - (UiMetrics::kToolbarShellMargin * 2.0f),
        UiMetrics::kToolbarShellHeight
    };
    DrawRectangleRounded(toolbarRect, 0.16f, 16, Fade(NookCol::UI_SHELL, 0.98f));
    DrawRectangleRoundedLinesEx(toolbarRect, 0.16f, 16, 2.0f, Fade(NookCol::UI_BORDER_SOFT, 0.50f));
    DrawRectangleRounded({ toolbarRect.x + 1.0f, toolbarRect.y + 1.0f, toolbarRect.width - 2.0f, 28.0f }, 0.12f, 16, Fade(WHITE, 0.04f));

    textRenderer->DrawSimpleText("Library", { UiMetrics::kToolbarLibraryX, UiMetrics::kToolbarLabelY }, UiMetrics::kToolbarLabelFontSize, Fade(NookCol::UI_TEXT, 0.90f));
    textRenderer->DrawSimpleText("Search", { m_ScreenWidth / 2.0f - UiMetrics::kToolbarSearchHalfLabelWidth, UiMetrics::kToolbarLabelY }, UiMetrics::kToolbarLabelFontSize, Fade(NookCol::UI_TEXT, 0.90f));
    textRenderer->DrawSimpleText("Actions", { (float)m_ScreenWidth - UiMetrics::kToolbarActionsInset, UiMetrics::kToolbarLabelY }, UiMetrics::kToolbarLabelFontSize, Fade(NookCol::UI_TEXT, 0.90f));

    DrawHelpText(textRenderer);
    textRenderer->DrawSimpleText(std::to_string(GetFPS()), { 10, (float)m_ScreenHeight - 20 }, 20, NookCol::UI_ACCENT_SOFT);

    for (auto& w : m_Widgets) w->Draw(textRenderer);

    if (graphRenderer != nullptr) {
        std::string count = "Nodes: " + std::to_string(graphRenderer->getNodes().size());
        textRenderer->DrawSimpleText(count, { 10, (float)m_ScreenHeight - 40 }, 20, NookCol::UI_TEXT_MUTED);
    }

    DrawNotification(textRenderer);
}

void UIManager::DrawHelpText(TextRenderer* renderer) const
{
    renderer->DrawSimpleText("Right-click drag: Move | Space: Add Books", { UiMetrics::kHelpX, UiMetrics::kHelpStartY + (UiMetrics::kHelpLineStep * 0.0f) }, UiMetrics::kDefaultFontSize, NookCol::UI_TEXT_MUTED);
    renderer->DrawSimpleText("Shift+Drag: Lock | Shift+Click: Delete", { UiMetrics::kHelpX, UiMetrics::kHelpStartY + (UiMetrics::kHelpLineStep * 1.0f) }, UiMetrics::kDefaultFontSize, NookCol::UI_TEXT_MUTED);
    renderer->DrawSimpleText("Middle Click: Pan | Scroll: Zoom", { UiMetrics::kHelpX, UiMetrics::kHelpStartY + (UiMetrics::kHelpLineStep * 2.0f) }, UiMetrics::kDefaultFontSize, NookCol::UI_TEXT_MUTED);
    renderer->DrawSimpleText("Double Click: Unlock Node | 'E': Edit Node", { UiMetrics::kHelpX, UiMetrics::kHelpStartY + (UiMetrics::kHelpLineStep * 3.0f) }, UiMetrics::kDefaultFontSize, NookCol::UI_TEXT_MUTED);
    renderer->DrawSimpleText("Ctrl+Z: Undo | Ctrl+Y: Redo", { UiMetrics::kHelpX, UiMetrics::kHelpStartY + (UiMetrics::kHelpLineStep * 4.0f) }, UiMetrics::kDefaultFontSize, NookCol::UI_TEXT_MUTED);
    renderer->DrawSimpleText("V: Unlock FPS | B: Enable VSync", { UiMetrics::kHelpX, UiMetrics::kHelpStartY + (UiMetrics::kHelpLineStep * 5.0f) }, UiMetrics::kDefaultFontSize, NookCol::UI_TEXT_MUTED);
}

void UIManager::DrawTooltip(Vector2 mousePos, TextRenderer* renderer) const
{
    if (m_LastHoveredNodeId != -1 && (GetTime() - m_HoverStartTime >= 0.5) && !IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        DrawRectangleRounded({ mousePos.x + UiMetrics::kTooltipOffset, mousePos.y + UiMetrics::kTooltipOffset, (float)m_CachedBoxWidth + 20, (float)m_CachedBoxHeight + 20 }, 0.2f, 10, Fade(NookCol::POPUP_BORDER, 0.75f));
        DrawRectangleRounded({ mousePos.x + UiMetrics::kTooltipOffset, mousePos.y + UiMetrics::kTooltipOffset, (float)m_CachedBoxWidth + 16, (float)m_CachedBoxHeight + 16 }, 0.2f, 10, Fade(NookCol::POPUP_BG, 0.95f));

        float xStart = mousePos.x + UiMetrics::kTooltipTextInset;
        float yStart = mousePos.y + UiMetrics::kTooltipTextInset;
        float spacing = UiMetrics::kTooltipLineSpacing;

        for (const auto& line : m_CachedLines) {
            renderer->DrawSimpleText(line, { xStart, yStart }, 20, NookCol::TEXT_DEFAULT);
            yStart += spacing;
        }
    }
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
        if (!w->isVisible) {
            continue;
        }

        if (w->isHovered || CheckCollisionPointRec(mouse, w->m_Bounds)) {
            return true;
        }
    }
    return false;
}
