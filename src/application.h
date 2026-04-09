
// Main application class managing the complete lifecycle of the NookLink app.
// Coordinates the book management system, UI, graph visualization, and I/O.
// Maintains application state machine (StartScreen, Editor) and undo/redo history.


#pragma once
#include "raylib.h"
#include "bookManager.h"
#include "connectionManager.h"
#include "graphManager.h"
#include "cameraHandler.h"
#include "nodeRenderer.h"
#include "uiManager.h"
#include "colors.h"
#include <string>
#include <vector>
#include <filesystem>
#include <memory>
#include "textRenderer.h"
#include "inputHandler.h"
#include "debugManager.h"
#include "UI/button.h"


enum class AppState
{
	StartScreen,
	Editor
};
class Application
{
public:
	Application();
	~Application(); 
	void Initialize();
	void Run();
	void Shutdown();
	

private:
	enum class HistoryActionType {
		AddBook,
		EditBook,
		DeleteBook
	};

	struct HistoryAction {
		HistoryActionType type;
		int bookId;
		Book before;
		Book after;
	};

	Vector2 m_ScreenSize{ 1920, 1080 };
	Vector2 m_CanvasSize{ 2000,2000 };

	
	std::unique_ptr<TextRenderer> m_TextRenderer; 
	BookManager m_BookManager{};
	ConnectionManager m_ConnectionManager{};
	std::unique_ptr<GraphManager> m_GraphManager{};
	std::unique_ptr<CameraHandler> m_CameraHandler{};
	std::unique_ptr<UIManager> m_UIManager{};

	std::unique_ptr<InputHandler> m_InputHandler{};
	std::unique_ptr<DebugManager> m_DebugManager{};
	
	std::filesystem::path m_SaveFileName{ "my_books.json" };

	bool m_IsUserInteracting{ false };

	bool m_HasUnsavedChanges{ false };

	int m_ReadingGoalTarget{ 12 };
	int m_ReadingGoalBaselineRead{ 0 };
	int m_ThemePresetIndex{ 0 };
	float m_LayoutDensityScale{ 1.0f };

	float m_UpdateInterval{ 0 };
	bool m_LayoutDirty{ true };
	const float m_UpdateIntervalInitial{ 0.001f };
	const int m_MaxSettleIterations{ 1000 };
	int m_SettleIterations{0 };




	AppState m_AppState{ AppState::StartScreen };

	std::shared_ptr<Button> m_BtnNewGraph;
	std::shared_ptr<Button> m_BtnLoadGraph;

	std::shared_ptr<Button> m_BtnContinue;

	bool m_HasShutdown{ false };
	std::vector<HistoryAction> m_UndoHistory{};
	std::vector<HistoryAction> m_RedoHistory{};
	static constexpr size_t m_MaxHistoryEntries{ 20 };

	struct FrameProfile {
		float updatePhysicsMs = 0.0f;
		float drawEdgesMs = 0.0f;
		float drawNodesMs = 0.0f;
		float drawUiMs = 0.0f;
		float frameCpuMs = 0.0f;
	};

	bool m_ShowProfilingOverlay = false;
	FrameProfile m_ProfileCurrent{};
	FrameProfile m_ProfileSmooth{};
	float m_ProfileSmoothing = 0.15f;

	void Update();
	void Draw();
	void DrawProfilingOverlay();
	void SmoothProfile();


	void LoadConfig();
	void SaveConfig();
	void ClearHistory();
	void PushHistoryAction(const HistoryAction& action);
	bool ApplyBookSnapshot(const Book& snapshot);
	bool UndoLastAction();
	bool RedoLastAction();

	void UpdateStartScreen();
	void DrawStartScreen();

	int GetReadBooksCount() const;
	void AdjustReadingGoalTarget(int delta);
	int GetReadingGoalProgress() const;
	void ResetReadingGoalProgressBaseline();


	
	


};