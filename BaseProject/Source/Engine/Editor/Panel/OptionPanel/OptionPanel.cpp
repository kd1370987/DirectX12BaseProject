#include "OptionPanel.h"

#include "../../../Option/OptionManager.h"

//==========================================================================================
// OptionPanel
//
// プロジェクト全体に効く設定(OptionManager)を並べるだけのパネル。
//
// 環境光・平行光・フォグ・空は「シーンごとに変わるもの」なので、ここではなく
// シーンに置く SceneAmbientObject が持つ。編集はそのオブジェクトのインスペクターから。
//==========================================================================================
void Engine::Editor::OptionPanel::OnDrawImGui(EditorContext& a_editContext)
{
	// オプションマネジャー
	Option::OptionManager::GetInstance().DrawEdit();
}
