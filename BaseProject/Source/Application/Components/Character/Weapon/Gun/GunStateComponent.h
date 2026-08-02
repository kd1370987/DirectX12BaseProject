#pragma once

#include "Engine/Editor/Helper/EditorHelper.h"

#include "../../../../Editor/CompEditHelper/CompEditHelper.h"

// 銃(発射体)の設定を持つコンポーネント。
// 「どのプレハブを・どれくらいの初速で撃つか」を保持する。
struct GunStateComponent
{
	float speed = 20.0f;			// 初速
	bool  isAuto = false;			// フルオート(押しっぱなしで撃ち続ける)かどうか
	float fireRate = 10.0f;			// 発射レート(1秒あたりの発射数)。フルオート時の連射間隔に使う

	// 発射するプレハブ
	Engine::GUID bulletPrefabGUID = {};									// 記録用(セーブされる)
	Engine::Handle<Engine::Resource::Prefab> bulletPrefabHandle = {};	// ランタイム用(発射時に解決)

	bool  prevShoot = false;		// 前フレームの発射入力(単発のエッジ検出用。保存しない)
	float shootCoolTime = 0.0f;		// 次に撃てるまでの残り時間(保存しない)

	UINT nullPtrNodeHash = 0;		// モデルのヌルポイント名ハッシュ値
	UINT nodeIndex = 0;				// ランタイム用ノードインデックス
};

template<>
struct Engine::ECS::ComponentTraits<GunStateComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		GunStateComponent& _comp = Engine::Editor::GetValue<GunStateComponent>(a_pData);
		a_ar.Field("speed", _comp.speed);
		a_ar.Field("isAuto", _comp.isAuto);
		a_ar.Field("fireRate", _comp.fireRate);
		a_ar.Field("bulletPrefabGUID", _comp.bulletPrefabGUID);
		a_ar.Field("nullPtrNodeHash", _comp.nullPtrNodeHash);
	}

	static void Edit(CompEditContext& a_context)
	{
		GunStateComponent& _comp = Engine::Editor::GetValue<GunStateComponent>(a_context.pData);

		ImGui::DragFloat("Speed", &_comp.speed, 0.1f, 0.0f);
		ImGui::Checkbox("IsAuto", &_comp.isAuto);

		// 発射レート(発/秒)。フルオートの時だけ効くので、単発時は無効表示にする
		ImGui::BeginDisabled(!_comp.isAuto);
		if (ImGui::DragFloat("Fire Rate", &_comp.fireRate, 0.1f, 0.01f, 1000.0f, "%.2f /s"))
		{
			if (_comp.fireRate < 0.01f) _comp.fireRate = 0.01f;
		}
		ImGui::EndDisabled();

		// 発射するプレハブの選択(アセットDBの Prefab 一覧から)
		if (Engine::Editor::EditorHelper::DrawAssetSelectComboGUID("Bullet Prefab", "Prefab", _comp.bulletPrefabGUID))
		{
			// GUIDが変わったらハンドルは作り直す(発射時に再解決)
			_comp.bulletPrefabHandle = {};
		}

		App::Editor::CompEditHelper::SelectSelfModelNode(
			a_context,
			_comp.nullPtrNodeHash,
			_comp.nodeIndex
		);
	}
};
