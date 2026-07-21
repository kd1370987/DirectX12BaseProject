#pragma once

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "Engine/Editor/EditorUI/EditorUI.h"

// CollisionEvent がヒットしたときの反応を設定するコンポーネント。
// ・爆発/エフェクトプレハブを当たった位置に生成する
// ・自分を消すか
struct ExplodeOnHitComponent
{
	Engine::GUID explosionPrefabGUID = {};									// 生成するプレハブ(任意)
	Engine::Handle<Engine::Resource::Prefab> explosionPrefabHandle = {};	// ランタイム用
	bool destroySelf = true;												// 当たったら自分を消すか
};

template<>
struct Engine::ECS::ComponentTraits<ExplodeOnHitComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		ExplodeOnHitComponent& _comp = Engine::Editor::GetValue<ExplodeOnHitComponent>(a_pData);
		a_ar.Field("explosionPrefabGUID", _comp.explosionPrefabGUID);
		a_ar.Field("destroySelf", _comp.destroySelf);
	}

	static void Edit(CompEditContext& a_context)
	{
		ExplodeOnHitComponent& _comp = Engine::Editor::GetValue<ExplodeOnHitComponent>(a_context.pData);

		if (Engine::Editor::UI::DrawAssetSelectComboGUID("Explosion Prefab", "Prefab", _comp.explosionPrefabGUID))
		{
			_comp.explosionPrefabHandle = {};	// GUIDが変わったら作り直し
		}
		ImGui::Checkbox("DestroySelf", &_comp.destroySelf);
	}
};
