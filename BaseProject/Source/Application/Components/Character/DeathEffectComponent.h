#pragma once

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "Engine/Editor/Helper/EditorHelper.h"

//==========================================================================================
// DeathEffectComponent
//
// 死んだときに出すエフェクトのプレハブを登録するコンポーネント。
// キャラクター(プレイヤー・敵)でも弾でも、消えるときに何か出したいものに付ける。
//
// ・出すのは DeathEffectSystem。「誰がどこで死んだか」は DeathEventResource 経由で受け取る。
//   死因を持っているシステム(体力切れなら HealthSystem、着弾なら ExplodeOnHitSystem)は
//   死亡を積むだけでよく、エフェクトの存在を知らなくて済む。
// ・登録するプレハブ側に ExplosionComponent や LifeTimeComponent を持たせておけば、
//   演出の進行と後片付けはエフェクト側で完結する。
//   こちらは「どのプレハブを、どこに出すか」だけを持つ。
//==========================================================================================
struct DeathEffectComponent
{
	Engine::GUID prefabGUID = {};									// 生成するプレハブ(未設定なら何も出ない)
	Engine::Handle<Engine::Resource::Prefab> prefabHandle = {};		// ランタイム用(生成時に解決)
};

template<>
struct Engine::ECS::ComponentTraits<DeathEffectComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		DeathEffectComponent& _comp = Engine::Editor::GetValue<DeathEffectComponent>(a_pData);
		a_ar.Field("prefabGUID", _comp.prefabGUID);
	}

	static void Edit(CompEditContext& a_context)
	{
		DeathEffectComponent& _comp = Engine::Editor::GetValue<DeathEffectComponent>(a_context.pData);

		if (Engine::Editor::EditorHelper::DrawAssetSelectComboGUID("Death Effect Prefab", "Prefab", _comp.prefabGUID))
		{
			_comp.prefabHandle = {};	// GUIDが変わったら作り直し
		}
	}
};
