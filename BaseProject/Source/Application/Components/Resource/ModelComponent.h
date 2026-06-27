#pragma once

#include "../../../Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../../Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "../../../Engine/Resource/Loader/Model/ModelLoader.h"

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
		auto* _pCurrentModel = Resource::ResourceManager::Instance().Ref(_comp.handle);;
		if (!_pCurrentModel) return;

		// 現在の表示
		ImGui::Text("Model : %s", _pCurrentModel->GetName().c_str());
		ImGui::Text("%s", _comp.modelGUID.String().c_str());

		// 選択UI
		if (ImGui::BeginCombo("Change Model", "Select..."))
		{
			for (auto& _prop : Resource::AssetDatabase::Instance().GetTypeMetaVec("Model"))
			{
				// 現在のモデルと同じGUIDなら選択中フラグを立てる
				bool _selected = (_comp.modelGUID == _prop.guid);

				// 選択欄
				if (ImGui::Selectable(_prop.fileName.c_str(), _selected))
				{
					// モデルのハンドル取得
					// ロードされていなかったら止まる
					_comp.handle = Resource::ResourceManager::Instance().Load<Resource::Model>(_prop.guid);
					_comp.modelGUID = _prop.guid;
				}
			}
			ImGui::EndCombo();
		}

		ImGui::Text("EmissiveScale");
		ImGui::ColorPicker4("EmissiveScale", (float*)&_comp.emissiveScale.x);

		ImGui::Text("ColorScale");
		ImGui::ColorPicker4("ColorScale", (float*)&_comp.colorScale.x);
	}
};