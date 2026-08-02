#pragma once

#include "../../../Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../../Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "../../../Engine/Editor/Helper/EditorHelper.inl"
namespace Engine::Resource
{
	class AnimatorAsset;
}

struct StateMachineComponent
{
	// 参照する設計図
	Engine::GUID stateMachineGUID = {};
	Engine::Handle<Engine::Resource::AnimatorAsset> stateMachineHandle = {};

	UINT prevStateHash = 0;			// 前回のステート
	UINT currentStateHash = 0;		// 現在のステート
	float currentTime = 0.0f;		// 現在のステートからの経過時間

	bool isGround = false;

	// ステートマシンインスタンス
	Engine::Handle<Engine::Resource::StateMachineInstance> instanceHandle = {};
};

template<>
struct Engine::ECS::ComponentTraits<StateMachineComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		StateMachineComponent& _comp = Engine::Editor::GetValue<StateMachineComponent>(a_pData);
		a_ar.Field("stateMachineGUID",_comp.stateMachineGUID);

	}

	static void Edit(CompEditContext& a_context)
	{
		using namespace Engine;
		StateMachineComponent& _comp = Engine::Editor::GetValue<StateMachineComponent>(a_context.pData);

		// ステートマシンの選択
		Editor::EditorHelper::DrawAssetSelectCombo<Resource::AnimatorAsset>(
			"Change StateMachine",
			"AnimatorAsset",
			_comp.stateMachineGUID,
			_comp.stateMachineHandle
		);

		// 現在のステートを表示
		const auto* _sm = Resource::ResourceManager::Instance().Get(_comp.stateMachineHandle);
		if (_sm)
		{
			std::string _nodeNameStr(_sm->GetNodeName(_comp.currentStateHash));
			ImGui::Text("Current Node : %s", _nodeNameStr.c_str());
		}
	}
};