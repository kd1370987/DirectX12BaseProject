#pragma once

#include "Engine/Editor/Helper/EditorHelper.h"

#include "../../../../Editor/CompEditHelper/CompEditHelper.h"

// 撃ち方
enum class EFireMode : uint32_t
{
	Auto,		// 押している間、fireRate の間隔で撃ち続ける
	Burst,		// 1回で burstCount 発を fireRate の間隔で撃ち、burstInterval あけて次のバーストへ
};

// 銃(発射体)の設定を持つコンポーネント。
// 「どのプレハブを・どれくらいの初速で・どう撃つか」を保持する。
//
// 単発撃ち(トリガーの立ち上がりで1発)は用意していない。
// 押しっぱなしの Auto を基本にして、ミサイルのようにまとめて撃つものは Burst を使う。
struct GunStateComponent
{
	float speed = 20.0f;			// 初速

	EFireMode fireMode = EFireMode::Auto;	// 撃ち方
	float fireRate = 10.0f;			// 発射レート(1秒あたりの発射数)。
									// Auto の連射間隔、Burst ではバースト内の間隔になる
	int   burstCount = 3;			// Burst : 1バーストで撃つ数
	float burstInterval = 1.0f;		// Burst : バーストを撃ち切ってから次のバーストまでの間隔(秒)

	// 発射するプレハブ
	Engine::GUID bulletPrefabGUID = {};									// 記録用(セーブされる)
	Engine::Handle<Engine::Resource::Prefab> bulletPrefabHandle = {};	// ランタイム用(発射時に解決)

	// ---- ランタイム(保存しない) ----

	// 前回発射してからの経過時間(秒)。撃った瞬間だけ 0 に戻し、あとは毎フレーム dt を足す。
	// ステート(アクションステートや構え)が変わってもリセットしない。
	// 撃っていない間も溜まり続けるので、久しぶりに引き金を引いたら即撃てる
	float timeSinceShoot = 0.0f;

	int   burstRemain = 0;			// Burst : 今のバーストで残っている発数

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
		a_ar.Field("fireMode", _comp.fireMode);
		a_ar.Field("fireRate", _comp.fireRate);
		a_ar.Field("burstCount", _comp.burstCount);
		a_ar.Field("burstInterval", _comp.burstInterval);
		a_ar.Field("bulletPrefabGUID", _comp.bulletPrefabGUID);
		a_ar.Field("nullPtrNodeHash", _comp.nullPtrNodeHash);
	}

	static void Edit(CompEditContext& a_context)
	{
		GunStateComponent& _comp = Engine::Editor::GetValue<GunStateComponent>(a_context.pData);

		ImGui::DragFloat("Speed", &_comp.speed, 0.1f, 0.0f);

		Engine::Editor::EditorHelper::DrawEnumCombo("FireMode", _comp.fireMode);

		// 発射レート(発/秒)。Auto は連射間隔、Burst はバースト内の間隔
		if (ImGui::DragFloat("Fire Rate", &_comp.fireRate, 0.1f, 0.01f, 1000.0f, "%.2f /s"))
		{
			if (_comp.fireRate < 0.01f) _comp.fireRate = 0.01f;
		}

		// バースト設定は Burst のときだけ効くので、それ以外は無効表示にする
		ImGui::BeginDisabled(_comp.fireMode != EFireMode::Burst);
		if (ImGui::DragInt("Burst Count", &_comp.burstCount, 1, 1, 100))
		{
			if (_comp.burstCount < 1) _comp.burstCount = 1;
		}
		if (ImGui::DragFloat("Burst Interval", &_comp.burstInterval, 0.05f, 0.0f, 60.0f, "%.2f s"))
		{
			if (_comp.burstInterval < 0.0f) _comp.burstInterval = 0.0f;
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

		// ---- ランタイム状態(参考) ----
		ImGui::Separator();
		ImGui::TextDisabled("TimeSinceShoot : %.2f s  BurstRemain : %d",
			_comp.timeSinceShoot, _comp.burstRemain);
	}
};
