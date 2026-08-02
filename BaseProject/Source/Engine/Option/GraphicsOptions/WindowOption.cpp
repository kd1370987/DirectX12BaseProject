#include "WindowOption.h"

#include "../../MainEngine.h"
#include "../../Window/NativeWindow.h"

void Engine::Option::GraphicsOptions::WindowOption::DrawEdit()
{
	// ウィンドウサイズ
	ImGui::Text("WindowSize");
	ImGui::Text("Width : %f", windowWidth);
	ImGui::Text("Height : %f", windowHeight);
	ImGui::DragInt("Width", &windowWidth, 1, 0, 1980);
	ImGui::DragInt("Height", &windowHeight, 1, 0, 1080);

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// ウィンドウタイトル
	if (ImGui::InputText("Title", &windowTitle))
	{
		// ウィンドウがない状況はあり得ないが一応
		auto* _pWindow = MainEngine::Instance().RefNativeWindow();
		if (_pWindow)
		{
			_pWindow->ChangeTitle(windowTitle);
		}
	}
	if (ImGui::Checkbox("IsTitleFPS", &isTitleFPS))
	{
		// FPS表示を消すため
		auto* _pWindow = MainEngine::Instance().RefNativeWindow();
		if (_pWindow)
		{
			_pWindow->ChangeTitle(windowTitle);
		}
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// ウィンドウモード
	if (Editor::EditorHelper::DrawEnumCombo("WindowMode", windowMode))
	{
		// ウィンドウがない状況はあり得ないが一応
		auto* _pWindow = MainEngine::Instance().RefNativeWindow();
		if (_pWindow)
		{
			_pWindow->ChangeWindowMode(windowMode);
		}
	}
	ImGui::Checkbox("Vsync", &isVsync);
	ImGui::DragInt("TargetFrameRate", &targetFrameRate, 1, 0, 1000);

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
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
