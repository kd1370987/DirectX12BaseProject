#include "WindowOption.h"

#include "../../MainEngine.h"
#include "../../Window/NativeWindow.h"

void Engine::Option::GraphicsOptions::WindowOption::DrawEdit(const ECS::EngineServices&)
{
	// ウィンドウサイズ
	Engine::Editor::Header("WindowSize");
	Engine::Editor::Value("Width", "%f", windowWidth);
	Engine::Editor::Value("Height", "%f", windowHeight);
	Engine::Editor::Field("Width", windowWidth, 1, 0, 1980);
	Engine::Editor::Field("Height", windowHeight, 1, 0, 1080);

	Engine::Editor::Line();

	// ウィンドウタイトル
	if (Engine::Editor::Field("Title", windowTitle))
	{
		// ウィンドウがない状況はあり得ないが一応
		auto* _pWindow = MainEngine::Instance().RefNativeWindow();
		if (_pWindow)
		{
			_pWindow->ChangeTitle(windowTitle);
		}
	}
	if (Engine::Editor::Field("IsTitleFPS", isTitleFPS))
	{
		// FPS表示を消すため
		auto* _pWindow = MainEngine::Instance().RefNativeWindow();
		if (_pWindow)
		{
			_pWindow->ChangeTitle(windowTitle);
		}
	}

	Engine::Editor::Line();

	// ウィンドウモード
	if (Engine::Editor::Field("WindowMode", windowMode))
	{
		// ウィンドウがない状況はあり得ないが一応
		auto* _pWindow = MainEngine::Instance().RefNativeWindow();
		if (_pWindow)
		{
			_pWindow->ChangeWindowMode(windowMode);
		}
	}
	Engine::Editor::Field("Vsync", isVsync);
	Engine::Editor::Field("TargetFrameRate", targetFrameRate, 1, 0, 1000);

	Engine::Editor::Line();
}

void Engine::Option::GraphicsOptions::WindowOption::Archive(Persistence::Archive& a_archive)
{
	a_archive.Field("WindowTitle", windowTitle);
	a_archive.Field("isTitleFPS",isTitleFPS);

	// ウィンドウサイズ
	a_archive.Field("windowWidth", windowWidth);
	a_archive.Field("windowHeight", windowHeight);

	// モード
	UINT _winMode = static_cast<UINT>(windowMode);
	a_archive.Field("windowMode", _winMode);
	windowMode = static_cast<EWindowMode>(_winMode);

	a_archive.Field("isVsync", isVsync);
	a_archive.Field("targetFrameRate", targetFrameRate);
}
