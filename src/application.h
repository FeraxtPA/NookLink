#pragma once
#include "raylib.h"
#include "bookManager.h"
#include "connectionManager.h"
#include "graphManager.h"
#include "cameraHandler.h"
#include "nodeRenderer.h"
#include "uiManager.h"
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

	float m_UpdateInterval{ 0 };
	bool m_LayoutDirty{ true };
	const float m_UpdateIntervalInitial{ 0.001f };
	const int m_MaxSettleIterations{ 1000 };
	int m_SettleIterations{0 };




	AppState m_AppState{ AppState::StartScreen };

	std::shared_ptr<Button> m_BtnNewGraph;
	std::shared_ptr<Button> m_BtnLoadGraph;

	std::shared_ptr<Button> m_BtnContinue;

	void Update();
	void Draw();


	void LoadConfig();
	void SaveConfig();

	void UpdateStartScreen();
	void DrawStartScreen();


	
	


};