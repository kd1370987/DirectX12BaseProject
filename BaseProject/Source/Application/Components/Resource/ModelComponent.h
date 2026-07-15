#pragma once

#include "../../../Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../../Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"

#include "../../../Engine/Editor/EditorUI/EditorUI.h"

struct ModelComponent
{
	DirectX::XMFLOAT4 colorScale = { 1.0f,1.0f,1.0f,1.0f };
	DirectX::XMFLOAT3 emissiveScale = { 1.0f,1.0f,1.0f };

	// モデル参照用
	Engine::Handle<Engine::Resource::Model> handle = {};	// ランタイム用
	Engine::GUID modelGUID = {};									// 記録用
};

template<>
struct Engine::ECS::ComponentTraits<ModelComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		ModelComponent& _comp = Engine::Editor::GetValue<ModelComponent>(a_pData);
		a_ar.Field("colorScale", _comp.colorScale);
		a_ar.Field("emissiveScale", _comp.emissiveScale);
		a_ar.Field("modelGUID", _comp.modelGUID);
	}

	static void Edit(void* a_pData)
	{

		ModelComponent& _comp = Engine::Editor::GetValue<ModelComponent>(a_pData);

		// ---------------------------------------------------------
		// モデルの選択UI
		// ---------------------------------------------------------
		if (Engine::Editor::UI::DrawAssetSelectCombo<Resource::Model>(
			"Change Model",
			"Model",
			_comp.modelGUID,
			_comp.handle))
		{
			// もしモデルが変更された時に特別な処理（再初期化など）が必要ならここに書ける
			// (戻り値が不要なら if文 で囲わなくてもOKです)
			// _comp.UpdateBoundingBox(); 
		}

		ImGui::Text("EmissiveScale");
		ImGui::ColorPicker4("EmissiveScale", (float*)&_comp.emissiveScale.x);

		ImGui::Text("ColorScale");
		ImGui::ColorPicker4("ColorScale", (float*)&_comp.colorScale.x);
	}
};