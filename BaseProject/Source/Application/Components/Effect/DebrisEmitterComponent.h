#pragma once

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Data/EffectPrefab/EffectPrefab.h"
#include "Engine/Editor/Helper/EditorField.h"
#include "Engine/ECS/World/World.h"

//==========================================================================================
// DebrisEmitterComponent
//
// 生まれた瞬間に、破片(EffectPrefab)を中心から外へ・上へ向けてまとめて撒く。
// 地面から飛び出す砂柱で岩を飛び散らせる、といった演出の「撒く側」。
//
// ・撒くのは DebrisEmitterSystem。1度だけ撒いて、あとは何もしない。
// ・撒かれる破片は EffectPrefab なので、破片も時間で必ず消える。
//   破片の動き(放物線・着地)は破片側の BallisticComponent、軌跡の砂埃は
//   破片側の EffectAssetComponent が受け持つ。ここが決めるのは初速と向きと回転だけ。
// ・向きは水平からの仰角(elevation)と、ぐるっと一周の中からの乱数で決める。
//   中心から startRadius だけ離した所から出すので、中心に重なって出ない。
// ・破片のハンドルは DebrisEmitterFixupSystem(PostDeserialize)が取る。
//   プレハブから写した値は持ち主ではないので、必ず取り直す。
//==========================================================================================
struct DebrisEmitterComponent
{
	// ---- 撒くもの(保存) ----
	Engine::GUID debrisGUID = Engine::DefaultGUID;							// 破片(EffectPrefab)
	Engine::Handle<Engine::Resource::EffectPrefab> debrisHandle = {};		// ランタイム用(Fixup が取る)

	// ---- 撒き方(保存) ----
	int   count           = 20;		// 撒く数
	float speedMin        = 15.0f;	// 初速の下限(m/秒)
	float speedMax        = 30.0f;	// 初速の上限(m/秒)
	float elevationMinDeg = 40.0f;	// 水平からの仰角の下限(度。90で真上)
	float elevationMaxDeg = 75.0f;	// 水平からの仰角の上限(度)
	float startRadius     = 3.0f;	// 中心から水平にこの距離だけ離して出す(m)
	float startHeight     = 1.0f;	// 中心からこの高さだけ上げて出す(m。地面に埋もれて出ないように)
	float spinMinDeg      = 90.0f;	// 回転の速さの下限(度/秒)
	float spinMaxDeg      = 540.0f;	// 回転の速さの上限(度/秒)
	float scaleMin        = 0.6f;	// 破片プレハブの大きさに掛ける倍率の下限
	float scaleMax        = 1.4f;	// 上限

	// ---- ランタイム(保存しない) ----
	bool isEmitted = false;			// 撒いたか
};

template<>
struct Engine::ECS::ComponentTraits<DebrisEmitterComponent>
{
	// コンポーネントはデストラクタが走らないので、参照を返すのはここの仕事
	static void Release(void* a_pData, const Engine::ECS::EngineServices& a_services)
	{
		DebrisEmitterComponent& _comp = Engine::Editor::GetValue<DebrisEmitterComponent>(a_pData);
		a_services.pResourceManager->ReleaseHandle(_comp.debrisHandle);
	}

	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		DebrisEmitterComponent& _comp = Engine::Editor::GetValue<DebrisEmitterComponent>(a_pData);
		a_ar.GUIDField("debrisGUID", _comp.debrisGUID);
		a_ar.Field("count", _comp.count);
		a_ar.Field("speedMin", _comp.speedMin);
		a_ar.Field("speedMax", _comp.speedMax);
		a_ar.Field("elevationMinDeg", _comp.elevationMinDeg);
		a_ar.Field("elevationMaxDeg", _comp.elevationMaxDeg);
		a_ar.Field("startRadius", _comp.startRadius);
		a_ar.Field("startHeight", _comp.startHeight);
		a_ar.Field("spinMinDeg", _comp.spinMinDeg);
		a_ar.Field("spinMaxDeg", _comp.spinMaxDeg);
		a_ar.Field("scaleMin", _comp.scaleMin);
		a_ar.Field("scaleMax", _comp.scaleMax);
	}

	static void Edit(CompEditContext& a_context)
	{
		DebrisEmitterComponent& _comp = Engine::Editor::GetValue<DebrisEmitterComponent>(a_context.pData);

		Engine::Editor::AssetField(
			*a_context.pWorld->RefEngineServices(), "Debris", "EffectPrefab", _comp.debrisGUID);
		if (_comp.debrisGUID == Engine::DefaultGUID)
		{
			Engine::Editor::HelpText("(未設定 : 何も撒かない)");
		}

		Engine::Editor::Field("Count", _comp.count, 1.0f, 0, 200);
		Engine::Editor::RangeField("Speed", _comp.speedMin, _comp.speedMax, 0.1f, 0.0f, 500.0f);
		Engine::Editor::RangeField("Elevation", _comp.elevationMinDeg, _comp.elevationMaxDeg, 0.5f, -90.0f, 90.0f);
		Engine::Editor::Field("Start Radius", _comp.startRadius, 0.05f, 0.0f);
		Engine::Editor::Field("Start Height", _comp.startHeight, 0.05f);
		Engine::Editor::RangeField("Spin", _comp.spinMinDeg, _comp.spinMaxDeg, 1.0f, 0.0f, 3600.0f);
		Engine::Editor::RangeField("Scale", _comp.scaleMin, _comp.scaleMax, 0.01f, 0.0f, 10.0f);
		Engine::Editor::HelpText("仰角は水平から(90で真上)。回転の軸は1個ずつ乱数");
	}
};
