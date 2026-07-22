#pragma once

#include "../../../Engine/Editor/ImGui/ImGuiHelper/ImGuiHelper.h"
#include "Engine/Scene/SceneManager\SceneManager.h"
#include "../../../Engine/ECS/World/World.h"
#include "../../../Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "../Resource/ModelComponent.h"
#include "HierarchyComponent.h"

//==============================================================================
// ヒエラルキー(HierarchyComponent)で親に紐づいたうえで、
// 親モデルの特定アニメーションノードへ追従したい、という意思を表すコンポーネント。
//   - 親エンティティは保持せず、HierarchyComponent::parentID から取得する。
//   - オフセットは持たない(ノードのワールドにそのまま追従する)。
//==============================================================================
struct FollowAnimationNodeComponent
{
	// 追従するノードのID
	UINT targetNodeHash = 0;		// ノード名のストリングハッシュ値(シリアライズ用)
	UINT targetNodeIdx = 0;			// ランタイム用ノードインデックス

	// ノード基準のオフセット
	DirectX::XMFLOAT3 offsetPosition = { 0, 0, 0 };
	DirectX::XMFLOAT4 offsetRotation = { 0, 0, 0, 1 };
	DirectX::XMFLOAT3 offsetScale = { 0, 0, 0 };
};


template<>
struct Engine::ECS::ComponentTraits<FollowAnimationNodeComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		FollowAnimationNodeComponent& _comp = Engine::Editor::GetValue<FollowAnimationNodeComponent>(a_pData);
		a_ar.Field("targetNodeHash", _comp.targetNodeHash);
		a_ar.Field("offsetPosition", _comp.offsetPosition);
		a_ar.Field("offsetRotation", _comp.offsetRotation);
		a_ar.Field("offsetScale", _comp.offsetScale);
	}

	static void Edit(CompEditContext& a_context)
	{
		FollowAnimationNodeComponent& _comp = Engine::Editor::GetValue<FollowAnimationNodeComponent>(a_context.pData);

		auto* _pWorld = a_context.pWorld
			? a_context.pWorld
			: Engine::Scene::SceneManager::Instance().RefWorld();

		ImGui::Text("TargetNodeIdx  : %d", _comp.targetNodeIdx);
		ImGui::Text("TargetNodeHash : %d", _comp.targetNodeHash);
		ImGui::Separator();

		// ノード基準のオフセット(位置・回転)
		ImGui::DragFloat3("OffsetPos", &_comp.offsetPosition.x, 0.1f);
		Engine::Editor::Helper::DragRotationDeg3FromQuaternion(_comp.offsetRotation);
		ImGui::DragFloat3("OffsetScalse", &_comp.offsetScale.x, 0.1f);
		ImGui::Separator();

		// 親エンティティはヒエラルキーから取得する
		Engine::ECS::Entity _parentID = Engine::ECS::Limits::INVALID_ENTITY;
		if (_pWorld && a_context.entity != Engine::ECS::Limits::INVALID_ENTITY)
		{
			auto* _pHierarchy = _pWorld->RefData<HierarchyComponent>(a_context.entity);
			if (_pHierarchy) _parentID = _pHierarchy->parentID;
		}

		if (!_pWorld || _parentID == Engine::ECS::Limits::INVALID_ENTITY)
		{
			ImGui::TextColored(ImVec4(1, 1, 0, 1), "Warning: No parent via HierarchyComponent.");
			return;
		}

		// 親のモデルコンポーネントを取得
		auto* _pParentModelComp = _pWorld->RefData<ModelComponent>(_parentID);
		if (!_pParentModelComp)
		{
			ImGui::TextColored(ImVec4(1, 1, 0, 1), "Warning: ModelComponent not found on Parent.");
			return;
		}

		// リソースマネージャーから実際のモデルを取得
		const auto* _pParentModel = Engine::Resource::ResourceManager::Instance().Get(_pParentModelComp->handle);
		if (!_pParentModel)
		{
			ImGui::TextColored(ImVec4(1, 1, 0, 1), "Warning: Model Resource is null.");
			return;
		}

		// モデルが管理する全ノード配列を取得
		const auto& _nodes = _pParentModel->GetOriginalNodeVec();

		// 現在選択されているノード名を表示用として取得
		std::string _currentNodeName = "None / Invalid";
		if (_comp.targetNodeIdx < _nodes.size())
		{
			_currentNodeName = _nodes[_comp.targetNodeIdx].name;
		}

		// ImGuiのコンボボックスで選択可能にする
		if (ImGui::BeginCombo("Target Node", _currentNodeName.c_str()))
		{
			for (size_t _i = 0; _i < _nodes.size(); ++_i)
			{
				bool _isSelected = (_comp.targetNodeIdx == _i);

				if (ImGui::Selectable(_nodes[_i].name.c_str(), _isSelected))
				{
					_comp.targetNodeHash = _nodes[_i].nodeNameHash;
					_comp.targetNodeIdx = static_cast<UINT>(_i);
				}

				if (_isSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}
};
