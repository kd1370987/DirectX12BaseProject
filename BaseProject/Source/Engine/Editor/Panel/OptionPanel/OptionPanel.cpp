#include "OptionPanel.h"

#include "../../../Option/OptionManager.h"

//==========================================================================================
// OptionPanel
//
// プロジェクト全体に効く設定(OptionManager)を並べるだけのパネル。
//
// 環境光・平行光・フォグ・空は「シーンごとに変わるもの」なので、ここではなく
// シーン(Engine::Scene::SceneAmbient)が持つ。編集は SceneAmbientPanel から。
//==========================================================================================
void Engine::Editor::OptionPanel::OnDrawImGui(EditorContext& a_editContext)
{
	// オプションマネジャー
	if (a_editContext.pServices) Option::OptionManager::Instance().DrawEdit(*a_editContext.pServices);
}
