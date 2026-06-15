#pragma once

#include "../../../Engine/Resource/Loader/StateMachineAsset/StateMachineAssetLoader.h"
#include "../../../Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../../Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "../../../Engine/Editor/ImGui/ImGuiHelper/ImGuiHelper.h"
namespace Engine::Resource
{
	class StateMachineAsset;
}

struct StateMachineComponent
{
	// 参照する設計図
	Engine::GUID stateMachineGUID = {};
	Engine::Resource::Handle<Engine::Resource::StateMachineAsset> stateMachineHandle = {};

	UINT prevStateHash = 0;			// 前回のステート
	UINT currentStateHash = 0;		// 現在のステート
	float currentTime = 0.0f;		// 現在のステートからの経過時間

	bool isGround = false;

	// ステートマシンインスタンス
	Engine::Resource::Handle<Engine::Resource::StateMachinInstance> instanceHandle = {};


	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const StateMachineComponent*>(a_ptr);
		a_json["stateMachineGUID"] = _comp->stateMachineGUID.String();
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<StateMachineComponent*>(a_ptr);
		_comp->stateMachineGUID.FromString(a_json["stateMachineGUID"].get<std::string>());
	}

	static void Edit(void* a_data)
	{
		using namespace Engine;
		StateMachineComponent& _comp = Engine::Editor::GetValue<StateMachineComponent>(a_data);
		
		// ステートマシンの選択
		if (ImGui::BeginCombo("Change StateMachin", "Select..."))
		{
			for (auto& _prop : Resource::AssetDatabase::Instance().GetTypeMetaVec("StateMachinAsset"))
			{
				// 現在のステートマシンと同じGUIDなら選択中フラグを立てる
				bool _selected = (_comp.stateMachineGUID == _prop.guid);

				// 選択欄
				if (ImGui::Selectable(_prop.fileName.c_str(), _selected))
				{
					_comp.stateMachineHandle = Resource::StateMachineAssetLoader::Load(_prop.guid);
					_comp.stateMachineGUID = _prop.guid;
				}
			}
			ImGui::EndCombo();
		}
	}
};