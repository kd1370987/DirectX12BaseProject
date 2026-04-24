#pragma once

#include "../../../Engine/Resource/Manager/ModelManager/ModelManager.h"

struct ModelComponent
{
	DirectX::XMFLOAT4 colorScale = { 1.0f,1.0f,1.0f,1.0f };
	DirectX::XMFLOAT3 emissiveScale = { 1.0f,1.0f,1.0f };

	// ランタイム用
	Engine::Resource::Handle<Engine::Resource::Model> handle = {};
	// 記録用
	//UUID modelGUID = {};
	Engine::GUID modelGUID = {};

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const ModelComponent*>(a_ptr);
		a_json["colorScale"] = { _comp->colorScale.x,_comp->colorScale.y ,_comp->colorScale.z ,_comp->colorScale.w };
		a_json["emissiveScale"] = { _comp->emissiveScale.x,_comp->emissiveScale.y ,_comp->emissiveScale.z};
		a_json["modelGUID"] = _comp->modelGUID.String();
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<ModelComponent*>(a_ptr);
		auto _colorScale = a_json.at("colorScale");
		_comp->colorScale.x = _colorScale[0].get<float>();
		_comp->colorScale.y = _colorScale[1].get<float>();
		_comp->colorScale.z = _colorScale[2].get<float>();
		_comp->colorScale.w = _colorScale[3].get<float>();

		_comp->modelGUID.FromString(a_json["modelGUID"].get<std::string>());
	}

	static void Edit(void* a_data)
	{
		using namespace Engine;
		ModelComponent& _comp = Engine::Editor::GetValue<ModelComponent>(a_data);
		auto* _pCurrentModel = Resource::ModelManager::Instnace().RefModel(_comp.handle);
		if (!_pCurrentModel) return;

		// 現在の表示
		ImGui::Text("Model : %s", _pCurrentModel->GetName().c_str());

		// 選択UI
		if (ImGui::BeginCombo("Change Model", "Select..."))
		{
			for (auto& [_guid, _handle] : Resource::ModelManager::Instnace().GetAllModelHandleMap())
			{
				// 選択中のモデルだったらフラグを立てる
				bool _selected = (_comp.handle == _handle);

				// 選択予定モデルの取得
				auto _pModel = Resource::ModelManager::Instnace().GetModel(_handle);

				// 選択欄
				if (ImGui::Selectable(_pModel->GetName().c_str(), _selected))
				{
					_comp.handle = _handle;
				}
			}
			ImGui::EndCombo();
		}

		ImGui::Text("EmissiveScale");
		ImGui::ColorPicker4("Color", (float*)&_comp.emissiveScale.x);

		ImGui::Text("ColorScale");
		ImGui::ColorPicker4("Color", (float*)&_comp.colorScale.x);
	}
};