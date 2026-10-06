#pragma once

#include "Engine/EditorField/EditorField.h"
#include "Engine/Scene/SceneManager/SceneManager.h"
#include "Engine/ECS/World/World.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Application/Components/Render/ModelComponent.h"
#include "HierarchyComponent.h"

#include "Application/Editor/CompEditHelper/CompEditHelper.h"

namespace App::Component
{
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
		Math::Vector3 offsetPosition = { 0, 0, 0 };
		Math::Quaternion offsetRotation = { 0, 0, 0, 1 };
		Math::Vector3 offsetScale = { 0, 0, 0 };
	};
}

template<>
struct Engine::ECS::ComponentTraits<App::Component::FollowAnimationNodeComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		App::Component::FollowAnimationNodeComponent& _comp = Engine::EditorField::RefValue<App::Component::FollowAnimationNodeComponent>(a_pData);
		a_ar.Field("targetNodeHash", _comp.targetNodeHash);
		a_ar.Field("offsetPosition", _comp.offsetPosition);
		a_ar.Field("offsetRotation", _comp.offsetRotation);
		a_ar.Field("offsetScale", _comp.offsetScale);
	}

	static void Edit(CompEditContext& a_context)
	{
		App::Component::FollowAnimationNodeComponent& _comp = Engine::EditorField::RefValue<App::Component::FollowAnimationNodeComponent>(a_context.pData);

		Engine::EditorField::Value("TargetNodeIdx", "%d", _comp.targetNodeIdx);
		Engine::EditorField::Value("TargetNodeHash", "%d", _comp.targetNodeHash);
		Engine::EditorField::Line();

		// ノード基準のオフセット(位置・回転)
		Engine::EditorField::Field("OffsetPos", _comp.offsetPosition, 0.1f);
		Engine::EditorField::Field("Rotation", _comp.offsetRotation);
		Engine::EditorField::Field("OffsetScalse", _comp.offsetScale, 0.1f);
		Engine::EditorField::Line();

		App::Editor::CompEditHelper::SelectParentModelNode(
			a_context,
			_comp.targetNodeHash,
			_comp.targetNodeIdx
		);
	}
};
