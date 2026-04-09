
// Main application runtime loop and event handling.
// Coordinates updates, input processing, and rendering loop.


#include "application.h"

#include "colors.h"
#include "logging.h"
#include "../include/tinyfiledialogs/tinyfiledialogs.h"

#include <algorithm>

namespace {
double NowSeconds() {
    return GetTime();
}
}

void Application::Run()
{
    bool exitApp = false;

    while (!exitApp)
    {
        // Centralized exit gate: keep all save-confirmation behavior in one place.
        if (WindowShouldClose())
        {
            if (!m_BookManager.getBooks().empty()) {
                if (m_HasUnsavedChanges)
                {
                    const int result = tinyfd_messageBox(
                        "Exit NookLink",
                        "Do you want to save your library before exiting?",
                        "yesnocancel",
                        "question",
                        1
                    );

                    if (result == 1) {
                        Log::Info("Saving session to: " + m_SaveFileName.string());
                        if (!m_BookManager.saveBooksToFile(m_SaveFileName.string())) {
                            m_UIManager->ShowNotification("Save before exit failed. Cancelled exit.");
                            exitApp = false;
                            continue;
                        }
                        exitApp = true;
                    }
                    else if (result == 2) {
                        exitApp = true;
                    }
                    else if (result == 0) {
                        exitApp = false;
                    }
                }
                else
                {
                    exitApp = true;
                }
            }
            else {
                exitApp = true;
            }
        }
        if (!exitApp)
        {
            Update();
            Draw();
        }
    }
}

void Application::Shutdown()
{
    if (m_HasShutdown) {
        return;
    }

    Log::Info("Application shutdown started");

    m_InputHandler.reset();
    m_DebugManager.reset();
    m_UIManager.reset();
    m_CameraHandler.reset();
    m_GraphManager.reset();
    m_TextRenderer.reset();

    if (IsWindowReady()) {
        CloseWindow();
    }

    Log::Info("Application shutdown finished");
    Log::Shutdown();

    m_HasShutdown = true;
}

void Application::UpdateStartScreen()
{
    if (m_BtnNewGraph) m_BtnNewGraph->Update();
    if (m_BtnLoadGraph) m_BtnLoadGraph->Update();

    if (m_BtnContinue) {
        const bool canContinue = !m_BookManager.getBooks().empty() || std::filesystem::exists(m_SaveFileName);
        if (canContinue) m_BtnContinue->Update();
    }
}

void Application::DrawStartScreen()
{
    const char* title = "NookLink";
    const int fontSize = 64;
    MeasureText(title, fontSize);
    m_TextRenderer->DrawTextCentered(title, { m_ScreenSize.x / 2.0f, m_ScreenSize.y / 4.0f }, fontSize, RAYWHITE);

    if (m_BtnNewGraph) m_BtnNewGraph->Draw(m_TextRenderer.get());
    if (m_BtnLoadGraph) m_BtnLoadGraph->Draw(m_TextRenderer.get());

    const bool canContinue = !m_BookManager.getBooks().empty() || std::filesystem::exists(m_SaveFileName);
    if (m_BtnContinue && canContinue) {
        m_BtnContinue->Draw(m_TextRenderer.get());
    }
}

void Application::Draw()
{
    const double frameCpuStart = NowSeconds();

    BeginDrawing();

    if (m_AppState == AppState::StartScreen)
    {
        ClearBackground(NookCol::BACKGROUND);
        DrawStartScreen();
    }
    else
    {
        ClearBackground(NookCol::BACKGROUND);

        m_CameraHandler->beginMode();

        const Rectangle viewRect = m_GraphManager->getCameraViewRect(m_CameraHandler->getCamera(), m_ScreenSize);

        const double edgesStart = NowSeconds();
        m_GraphManager->drawEdges(m_CameraHandler->getCamera().zoom, viewRect);
        m_ProfileCurrent.drawEdgesMs = (float)((NowSeconds() - edgesStart) * 1000.0);

        const double nodesStart = NowSeconds();
        m_GraphManager->drawNodes(m_CameraHandler->getCamera().zoom, viewRect);
        m_ProfileCurrent.drawNodesMs = (float)((NowSeconds() - nodesStart) * 1000.0);

        m_CameraHandler->endMode();

        const double uiStart = NowSeconds();
        m_UIManager->Draw(GetMousePosition(), m_GraphManager.get(), m_BookManager, m_TextRenderer.get());
        m_ProfileCurrent.drawUiMs = (float)((NowSeconds() - uiStart) * 1000.0);

        m_ProfileCurrent.frameCpuMs = (float)((NowSeconds() - frameCpuStart) * 1000.0);
        SmoothProfile();

        if (m_ShowProfilingOverlay) {
            DrawProfilingOverlay();
        }
    }

    EndDrawing();
}

void Application::Update()
{
    if (IsKeyPressed(KEY_F3)) {
        m_ShowProfilingOverlay = !m_ShowProfilingOverlay;
        Log::Info(std::string("Profiling overlay ") + (m_ShowProfilingOverlay ? "enabled" : "disabled"));
    }

    if (IsWindowResized()) {
        m_ScreenSize.x = (float)GetScreenWidth();
        m_ScreenSize.y = (float)GetScreenHeight();

        m_CameraHandler->updateScreenSize(m_ScreenSize);

        if (m_BtnContinue) m_BtnContinue->OnWindowResize(GetScreenWidth(), GetScreenHeight());
        if (m_BtnNewGraph) m_BtnNewGraph->OnWindowResize(GetScreenWidth(), GetScreenHeight());
        if (m_BtnLoadGraph) m_BtnLoadGraph->OnWindowResize(GetScreenWidth(), GetScreenHeight());

        m_UIManager->OnWindowResize(GetScreenWidth(), GetScreenHeight());
    }

    if (m_AppState == AppState::StartScreen)
    {
        UpdateStartScreen();
    }
    else
    {
        const Vector2 worldMousePos = GetScreenToWorld2D(GetMousePosition(), m_CameraHandler->getCamera());

        const bool ctrlDown = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
        if (ctrlDown && IsKeyPressed(KEY_Z)) {
            UndoLastAction();
        }
        else if (ctrlDown && IsKeyPressed(KEY_Y)) {
            RedoLastAction();
        }

        if (IsWindowFocused())
        {
            m_IsUserInteracting =
                IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) ||
                IsMouseButtonReleased(MOUSE_RIGHT_BUTTON) ||
                IsMouseButtonDown(MOUSE_RIGHT_BUTTON) ||
                IsMouseButtonPressed(MOUSE_LEFT_BUTTON) ||
                IsKeyPressed(KEY_SPACE) ||
                IsKeyPressed(KEY_V) ||
                IsKeyPressed(KEY_B);
        }

        if (m_LayoutDirty) {
            if (m_GraphManager) m_GraphManager->wakeUpPhysics();
            m_LayoutDirty = false;
        }

        if (m_GraphManager && m_AppState == AppState::Editor) {
            // Clamp dt to avoid unstable physics after frame stalls.
            const float dt = std::clamp(GetFrameTime(), 0.0f, 0.05f);

            const double physicsStart = NowSeconds();
            m_GraphManager->updatePhysics(dt);
            m_ProfileCurrent.updatePhysicsMs = (float)((NowSeconds() - physicsStart) * 1000.0);
        }

        m_UIManager->Update(
            worldMousePos,
            GetMousePosition(),
            m_BookManager,
            m_GraphManager.get(), m_TextRenderer.get());

        if (!m_UIManager->IsBlockingGraphInteraction())
        {
            bool consumedInput = false;

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && IsKeyDown(KEY_LEFT_SHIFT)) {
                Node* clickedNode = m_GraphManager->getNodeAtPosition(worldMousePos);
                if (clickedNode && clickedNode->type == NodeType::Book) {
                    const Book* toDelete = m_BookManager.findBookById(clickedNode->id);
                    if (toDelete) {
                        // Snapshot before delete so Ctrl+Z can fully restore the removed book.
                        PushHistoryAction(HistoryAction{ HistoryActionType::DeleteBook, clickedNode->id, *toDelete, Book() });
                        m_BookManager.removeBook(clickedNode->id);
                        m_GraphManager->initializePositions();
                        m_LayoutDirty = true;
                        m_HasUnsavedChanges = true;
                        m_SettleIterations = 0;
                        m_UIManager->ShowNotification("Book deleted (Shift+Click). Undo: Ctrl+Z");
                        Log::Info("Deleted book ID: " + std::to_string(clickedNode->id));
                        consumedInput = true;
                    }
                }
            }

            m_CameraHandler->update();
            if (!consumedInput) {
                m_InputHandler->ProcessInputs(worldMousePos, GetTime(), m_LayoutDirty, m_HasUnsavedChanges);
            }
            m_DebugManager->HandleDebugInputs(m_LayoutDirty);
        }
    }
}

void Application::SmoothProfile()
{
    // Exponential smoothing reduces frame-to-frame jitter in the on-screen profiler.
    auto smooth = [this](float current, float& smoothed) {
        smoothed += (current - smoothed) * m_ProfileSmoothing;
    };

    smooth(m_ProfileCurrent.updatePhysicsMs, m_ProfileSmooth.updatePhysicsMs);
    smooth(m_ProfileCurrent.drawEdgesMs, m_ProfileSmooth.drawEdgesMs);
    smooth(m_ProfileCurrent.drawNodesMs, m_ProfileSmooth.drawNodesMs);
    smooth(m_ProfileCurrent.drawUiMs, m_ProfileSmooth.drawUiMs);
    smooth(m_ProfileCurrent.frameCpuMs, m_ProfileSmooth.frameCpuMs);
}

void Application::DrawProfilingOverlay()
{
    if (m_AppState != AppState::Editor) return;

    const int x = 12;
    const int y = 320;
    const int w = 360;
    const int h = 174;
    const int fs = 20;
    const int lineStep = 24;

    DrawRectangleRounded({ (float)x, (float)y, (float)w, (float)h }, 0.15f, 10, Fade(BLACK, 0.65f));
    DrawRectangleRoundedLinesEx({ (float)x, (float)y, (float)w, (float)h }, 0.15f, 10, 2.0f, Fade(NookCol::UI_BORDER_SOFT, 0.85f));

    const float fps = GetFPS() > 0 ? (float)GetFPS() : 0.0f;
    const float frameBudgetMs = fps > 0.0f ? (1000.0f / fps) : 0.0f;

    DrawText(TextFormat("Profiling (F3)"), x + 12, y + 10, fs, RAYWHITE);
    DrawText(TextFormat("physics:   %6.2f ms", m_ProfileSmooth.updatePhysicsMs), x + 12, y + 10 + lineStep * 1, fs, RAYWHITE);
    DrawText(TextFormat("drawEdges: %6.2f ms", m_ProfileSmooth.drawEdgesMs), x + 12, y + 10 + lineStep * 2, fs, RAYWHITE);
    DrawText(TextFormat("drawNodes: %6.2f ms", m_ProfileSmooth.drawNodesMs), x + 12, y + 10 + lineStep * 3, fs, RAYWHITE);
    DrawText(TextFormat("drawUI:    %6.2f ms", m_ProfileSmooth.drawUiMs), x + 12, y + 10 + lineStep * 4, fs, RAYWHITE);
    DrawText(TextFormat("frameCPU:  %6.2f ms", m_ProfileSmooth.frameCpuMs), x + 12, y + 10 + lineStep * 5, fs, NookCol::UI_ACCENT_SOFT);
    DrawText(TextFormat("fps: %3i | budget: %5.2f ms", GetFPS(), frameBudgetMs), x + 12, y + 10 + lineStep * 6, fs, NookCol::UI_TEXT_MUTED);
    DrawText(TextFormat("nodes: %zu | edges: %zu", m_GraphManager->getNodes().size(), m_GraphManager->getEdgeCount()), x + 12, y + 10 + lineStep * 7, fs, NookCol::UI_TEXT_MUTED);
}
