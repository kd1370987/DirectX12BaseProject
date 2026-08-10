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
	ImGui::DragFloat3("AmbientColor",&_ambientData.ambientColorScale.x,0.01f);
	ImGui::DragFloat3("DLColor",&_ambientData.dlColor.x,0.01f);
	ImGui::DragFloat3("DLDir",&_ambientData.dlDir.x,0.01f);

	// ---- フォグ ----
	// enable が false の間はシェーダー側で計算ごとスキップされる
	ImGui::SeparatorText("HeightFog");
	{
		bool _enable = (_ambientData.heightFogEnable != 0);
		if (ImGui::Checkbox("HeightFogEnable", &_enable))
		{
			_ambientData.heightFogEnable = _enable ? 1 : 0;
		}

		ImGui::ColorEdit3("HeightFogColor", &_ambientData.heightFogColor.x);
		ImGui::DragFloat("HeightFogHeight", &_ambientData.heightFogHeight, 0.1f);
		// 基準高さからこの距離だけ進むと 100%
		ImGui::DragFloat("HeightFogMaxRange", &_ambientData.heightFogMaxRange, 0.1f, 0.0f);

		// どちら側へ濃くしていくか
		static const char* _denseName[] = { "Upward", "Downward" };
		int _denseDown = _ambientData.heightFogDenseDown != 0 ? 1 : 0;
		if (ImGui::Combo("HeightFogDense", &_denseDown, _denseName, IM_ARRAYSIZE(_denseName)))
		{
			_ambientData.heightFogDenseDown = _denseDown;
		}
	}

	ImGui::SeparatorText("DistanceFog");
	{
		bool _enable = (_ambientData.distanceFogEnable != 0);
		if (ImGui::Checkbox("DistanceFogEnable", &_enable))
		{
			_ambientData.distanceFogEnable = _enable ? 1 : 0;
		}

		ImGui::ColorEdit3("DistanceFogColor", &_ambientData.distanceFogColor.x);
		ImGui::DragFloat("DistanceFogStart", &_ambientData.distanceFogStart, 0.1f, 0.0f);
		// この距離で 100%。開始距離より手前には下げられないようにしておく
		ImGui::DragFloat("DistanceFogMaxRange", &_ambientData.distanceFogMaxRange, 0.1f,
			_ambientData.distanceFogStart, FLT_MAX);
	}
}
