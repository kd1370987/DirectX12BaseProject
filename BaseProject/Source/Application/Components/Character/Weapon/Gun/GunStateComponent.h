#pragma once

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "Engine/Editor/EditorUI/EditorUI.h"

// 銃(発射体)の設定を持つコンポーネント。
// 「どのプレハブを・どれくらいの初速で撃つか」を保持する。
struct GunStateComponent
{
	float speed = 20.0f;			// 初速
	bool  isAuto = false;			// フルオート(押しっぱなしで撃ち続ける)かどうか

	// 発射するプレハブ
	Engine::GUID bulletPrefabGUID = {};									// 記録用(セーブされる)
	Engine::Handle<Engine::Resource::Prefab> bulletPrefabHandle = {};	// ランタイム用(発射時に解決)

	bool  prevShoot = false;		// 前フレームの発射入力(単発のエッジ検出用。保存しない)
};

template<>
struct Engine::ECS::ComponentTraits<GunStateComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		GunStateComponent& _comp = Engine::Editor::GetValue<GunStateComponent>(a_pData);
		a_ar.Field("speed", _comp.speed);
		a_ar.Field("isAuto", _comp.isAuto);
		a_ar.Field("bulletPrefabGUID", _comp.bulletPrefabGUID);
	}

	static void Edit(CompEditContext& a_context)
	{
		GunStateComponent& _comp = Engine::Editor::GetValue<GunStateComponent>(a_context.pData);

		ImGui::DragFloat("Speed", &_comp.speed, 0.1f, 0.0f);
		ImGui::Checkbox("IsAuto", &_comp.isAuto);

		// 発射するプレハブの選択(アセットDBの Prefab 一覧から)
		if (Engine::Editor::UI::DrawAssetSelectComboGUID("Bullet Prefab", "Prefab", _comp.bulletPrefabGUID))
		{
			// GUIDが変わったらハンドルは作り直す(発射時に再解決)
			_comp.bulletPrefabHandle = {};
		}
	}
};
