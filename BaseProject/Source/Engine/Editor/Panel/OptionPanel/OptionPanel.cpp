#include "OptionPanel.h"

#include "../../../Option/OptionManager.h"
#include "../../../MainEngine.h"
#include "../../../Graphics/GraphicEngine.h"

void Engine::Editor::OptionPanel::OnDrawImGui(EditorContext& a_editContext)
{
	// オプションマネジャー
	Option::OptionManager::GetInstance().DrawEdit();

	ImGui::Separator();

	// グラフィックス設定
	auto* _pGE = MainEngine::Instance().RefGraphicsEngine();
	if (!_pGE) return;

	auto& _ambientData = _pGE->RefAmbientData();

	ImGui::DragFloat3("DLColor",&_ambientData.dlColor.x,0.01f);
	ImGui::DragFloat3("DLDir",&_ambientData.dlDir.x,0.01f);
}
