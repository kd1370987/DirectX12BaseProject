#pragma once
#include "../IPanel.h"
#include "Editor/EditorCommon.h"

namespace Editor
{
	//======================================================================================
	// SceneConfigPanel
	//
	// 今のシーン(一番上)の設定(Engine::Scene::SceneConfig)を触るパネル。
	// 先読み一覧の確認・手直しと、次に開いたときに一覧を計測し直すフラグを持つ。
	// シーンを保存したときに一緒に保存される(ファイルはシーンと別)。
	//======================================================================================
	class SceneConfigPanel : public IPanel
	{
	public:
		~SceneConfigPanel() override = default;

		const char* GetName() const override { return "SceneConfigPanel"; };
		void OnDrawImGui(EditorContext& a_editContext) override;
	};
}
