#pragma once

#include "../../../Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../../Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "../../../Engine/Editor/ImGui/ImGuiHelper/ImGuiHelper.h"
namespace Engine::Resource
{
	class ActionStateMachineAsset;
}

// ゲームプレイ用ステートマシンをエンティティに紐付けるコンポーネント。
// 「入力→パラメータ→状態→行動」の“状態”を保持する。
struct ActionStateComponent
{
	// 参照する設計図
	Engine::GUID actionGUID = {};
	Engine::Handle<Engine::Resource::ActionStateMachineAsset> actionHandle = {};

	UINT prevStateHash = 0;			// 前回のステート
	UINT currentStateHash = 0;		// 現在のステート
	float currentTime = 0.0f;		// 現在のステートからの経過時間

	// 実行時パラメータの実体
	Engine::Handle<Engine::Resource::ActionStateInstance> instanceHandle = {};
};

template<>
struct Engine::ECS::ComponentTraits<ActionStateComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		ActionStateComponent& _comp = Engine::Editor::GetValue<ActionStateComponent>(a_pData);
		a_ar.Field("actionGUID", _comp.actionGUID);
	}

	static void Edit(CompEditContext& a_context)
	{
		using namespace Engine;
		ActionStateComponent& _comp = Engine::Editor::GetValue<ActionStateComponent>(a_context.pData);

		// ステートマシンの選択
		if (ImGui::BeginCombo("Change ActionSM", "Select..."))
		{
			for (auto& _prop : Resource::AssetDatabase::Instance().GetTypeMetaVec("ActionStateMachineAsset"))
			{
				bool _selected = (_comp.actionGUID == _prop.guid);
				if (ImGui::Selectable(_prop.fileName.c_str(), _selected))
				{
					_comp.actionHandle = Resource::ResourceManager::Instance().Load<Resource::ActionStateMachineAsset>(_prop.guid);
					_comp.actionGUID = _prop.guid;
				}
			}
			ImGui::EndCombo();
		}

		// 現在のステートを表示
		const auto* _sm = Resource::ResourceManager::Instance().Get(_comp.actionHandle);
		if (_sm)
		{
			std::string _nodeNameStr(_sm->GetNodeName(_comp.currentStateHash));
			ImGui::Text("Current Node : %s", _nodeNameStr.c_str());
		}
	}
};
