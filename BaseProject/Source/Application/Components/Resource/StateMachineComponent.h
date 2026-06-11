#pragma once

#include "../../../Engine/Resource/Loader/StateMachineAsset/StateMachineAssetLoader.h"

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

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const StateMachineComponent*>(a_ptr);
		
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<StateMachineComponent*>(a_ptr);
		
	}

	static void Edit(void* a_data)
	{
		using namespace Engine;
		StateMachineComponent& _comp = Engine::Editor::GetValue<StateMachineComponent>(a_data);
		
		static char _guid[256] = "";
		ImGui::InputText("GUID",_guid,sizeof(_guid));
		_comp.stateMachineGUID.FromString(std::string(_guid));
	}
};