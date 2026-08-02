#pragma once

#include "../../../Engine/Editor/Helper/EditorHelper.h"
#include "Engine/Scene/SceneManager\SceneManager.h"
#include "../../../Engine/ECS/World/World.h"
#include "../../../Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "../Resource/ModelComponent.h"
#include "HierarchyComponent.h"


#include "../../Editor/CompEditHelper/CompEditHelper.h"

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

		ImGui::Text("TargetNodeIdx  : %d", _comp.targetNodeIdx);
		ImGui::Text("TargetNodeHash : %d", _comp.targetNodeHash);
		ImGui::Separator();

		// ノード基準のオフセット(位置・回転)
		ImGui::DragFloat3("OffsetPos", &_comp.offsetPosition.x, 0.1f);
		Engine::Editor::EditorHelper::DragRotationDeg3FromQuaternion(_comp.offsetRotation);
		ImGui::DragFloat3("OffsetScalse", &_comp.offsetScale.x, 0.1f);
		ImGui::Separator();

		App::Editor::CompEditHelper::SelectModelNode(
			a_context,
			_comp.targetNodeHash,
			_comp.targetNodeIdx
		);
		
	}
};
