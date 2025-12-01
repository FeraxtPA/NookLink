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

	

	BookManager m_BookManager{};
	ConnectionManager m_ConnectionManager{};
	std::unique_ptr<GraphManager> m_GraphManager{};
	std::unique_ptr<CameraHandler> m_CameraHandler{};
	std::unique_ptr<UIManager> m_UIManager{};

	
	
	std::filesystem::path m_SaveFileName{ "my_books.json" };

	bool m_IsUserInteracting{ false };

	float m_UpdateInterval{ 0 };
	bool m_LayoutDirty{ true };
	const float m_UpdateIntervalInitial{ 0.001f };
	const int m_MaxSettleIterations{ 1000 };
	int m_SettleIterations{0 };

	Node* m_LastClickedNode{ nullptr };
	float m_LastClickTime{ 0.0f };
	const float m_DoubleClickThreshold{ 0.3f };


	void Update();
	
	void InitBookManager();
	void Draw();
	void HandleInput(Vector2 worldMousePos);


};