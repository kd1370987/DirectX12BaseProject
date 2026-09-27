#pragma once

#include "BoostIntentComponent.h"
#include "BoostStateComponent.h"

//==========================================================================================
// BoostParamsComponent
//
// ブーストの設定。保存される。システムは読むだけ。
//
// ・以前の BoostComponent は入力・設定・燃料・踏み込みの状態を1つに持っていて、
//   入力の書き手(入力・AI・死亡ゲート)と状態の書き手(RobotBoost / ChargeDash)が
//   同じ型を読み書きし合い、依存が循環していた。
//     入力 … BoostIntentComponent(押した瞬間・押している)
//     状態 … BoostStateComponent(ブースト中か・燃料・踏み込み)
//   どちらも必須コンポーネントとして自動で付く。
//==========================================================================================
struct BoostParamsComponent
{
	// ブースト中の水平速度(m/秒)。進む向きは移動入力、入力が無ければ向いている方向。
	// 以前の「今の速度に掛ける倍率」ではないので注意(止まっていても飛べるようにするため)
	float boostPower = 30.0f;

	/// <summary>押した瞬間の速度倍率(継続ブーストに対する倍率)</summary>
	/// <remarks>
	/// 踏み込んだ瞬間がこの倍率で一番速く、tapBoostTime をかけて
	/// 継続ブーストの速さ(boostPower)まで落ちていく
	/// </remarks>
	float tapBoostScale = 2.0f;

	/// <summary>初動の勢いが残る秒数</summary>
	/// <remarks>
	/// 0 にすると初動が1フレームで終わる。
	/// 目標速度は実速度が追いかける作りなので、1フレームでは追いつく前に
	/// 元へ戻ってしまい、踏み込みが出ない
	/// </remarks>
	float tapBoostTime = 0.35f;
	float boostFuel = 5.0f;			// ブースト単押しの使用燃料
	float boostFuelPerSec = 1.0f;	// ブースト連続使用時の毎秒消費燃料量

	float maxFuel = 100.0f;			// 燃料の最大値
	float fuelRegeneration = 1.0f;	// 秒間回復量
};

template<>
struct Engine::ECS::ComponentTraits<BoostParamsComponent>
{
	// 入力と状態は実行中の値なので、プレハブに書かずに自動で付ける
	using Requires = Engine::ECS::RequireComponents<BoostIntentComponent, BoostStateComponent>;

	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		BoostParamsComponent& _comp = Engine::Editor::GetValue<BoostParamsComponent>(a_pData);
		a_ar.Field("maxFuel", _comp.maxFuel);
		a_ar.Field("boostPower", _comp.boostPower);
		a_ar.Field("tapBoostScale", _comp.tapBoostScale);
		a_ar.Field("tapBoostTime", _comp.tapBoostTime);
		a_ar.Field("boostFuel", _comp.boostFuel);
		a_ar.Field("boostFuelPerSec", _comp.boostFuelPerSec);
		a_ar.Field("fuelRegeneration", _comp.fuelRegeneration);
	}

	static void Edit(CompEditContext& a_context)
	{
		using namespace Engine;
		BoostParamsComponent& _comp = Engine::Editor::GetValue<BoostParamsComponent>(a_context.pData);

		Engine::Editor::Header("Boost Parameters");
		Engine::Editor::Field("Max Fuel", _comp.maxFuel, 1.0f, 0.0f);
		Engine::Editor::Field("Boost Power (m/s)", _comp.boostPower, 0.1f, 0.0f);
		Engine::Editor::Field("Tap Boost Scale", _comp.tapBoostScale, 0.05f, 0.0f);
		Engine::Editor::Field("Tap Boost Time", _comp.tapBoostTime, 0.01f, 0.0f);
		Engine::Editor::Tooltip("(踏み込みが続く秒数。0で1フレームだけ = ほぼ効かない)");
		Engine::Editor::Field("Boost Fuel (Tap)", _comp.boostFuel, 0.1f, 0.0f);
		Engine::Editor::Field("Boost Fuel / Sec", _comp.boostFuelPerSec, 0.1f, 0.0f);

		Engine::Editor::Field("FuelRegeneration", _comp.fuelRegeneration, 0.1f, 0.0f);
	}
};