#pragma once

#include "Engine/ECS/World/World.h"
#include "Application/InstanceResource/AdditiveBoneEntry.h"

namespace App::Component
{
	//==========================================================================================
	//
	// 加算ポーズ。
	//
	// 「どのボーンに加算するか」の構造は AnimatorAsset(AdditiveBoneDef)が持ち、
	// このコンポーネントは「どれくらい強く・どれくらい速く効かせるか」の調整値と、
	// 解決済みボーン配列へのハンドル、そして実行時の状態を持つ。
	//
	//==========================================================================================
	struct AdditivePoseComponent
	{
		// --- 解決済みボーン配列(AdditivePoseLinkSystem が確保・解決する) ---
		Engine::RangeHandle<InstanceResource::AdditiveBoneEntry> handle = {};

		// --- 調整値 ---
		float masterWeight	= 1.0f;		// エンティティ全体の効き(0で完全無効)
		float yawLimitDeg	= 60.0f;	// 上半身の可動域(左右)
		float pitchLimitDeg	= 35.0f;	// 上半身の可動域(上下)
		float followRate	= 12.0f;	// 照準追従のSlerp速度
		float lagStiffness	= 20.0f;	// 引っ張られのバネ定数
		float lagDamping	= 8.0f;		// 引っ張られの減衰
		float lagScale		= 0.02f;	// 速度→角度の変換係数(rad / (m/秒))
		float lagLimitDeg	= 25.0f;	// 引っ張られの最大角
		float lagArmScale	= 1.0f;		// LagArm チャンネルの倍率
		float lagLegScale	= 0.7f;		// LagLeg チャンネルの倍率

		// 空中(接地していない間・チャージダッシュ中)。空中用のチャンネル(AimArm / LagBody)を持つときだけ使う
		float airBlendRate	= 6.0f;		// 地上 ⇔ 空中の切り替えの速さ(1秒あたり)

		// 体全体の前のめり(LagBody)の最大角。動き方で変える
		float leanNormalDeg	= 20.0f;	// 通常の空中移動
		float leanBoostDeg	= 40.0f;	// ブースト中
		float leanDashDeg	= 85.0f;	// チャージダッシュ中(ほぼ進行方向と水平)
		float leanFullSpeed	= 3.0f;		// この水平速度(m/秒)で最大角に届く。遅いときは比例して浅くなる
		float leanStiffness	= 30.0f;	// 前のめりのバネ定数
		float leanDamping	= 11.0f;	// 前のめりの減衰

		// --- 実行時 ---
		// currentAimQuat は必ず単位クォータニオンで初期化すること。
		// ゼロクォータニオンを XMMatrixRotationQuaternion に渡すとスケール0の行列になり、
		// メッシュが原点に潰れる。
		Math::Quaternion currentAimQuat	= { 0.0f, 0.0f, 0.0f, 1.0f };	// 現在の上半身回転(補間後)
		Math::Vector3 lagAngle			= { 0.0f, 0.0f, 0.0f };			// バネの現在値(ラジアン)
		Math::Vector3 lagVelocity		= { 0.0f, 0.0f, 0.0f };			// バネの速度
		float airBlend					= 0.0f;							// 0 : 地上用のチャンネル / 1 : 空中用のチャンネル
		Math::Vector3 bodyLeanAngle		= { 0.0f, 0.0f, 0.0f };			// 前のめりのバネの現在値(ラジアン)
		Math::Vector3 bodyLeanVelocity	= { 0.0f, 0.0f, 0.0f };			// 前のめりのバネの速度
	};
}

template<>
struct Engine::ECS::ComponentTraits<App::Component::AdditivePoseComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		App::Component::AdditivePoseComponent& _comp = Engine::EditorField::RefValue<App::Component::AdditivePoseComponent>(a_pData);

		// 調整値のみ保存する。
		// ボーン構成は AnimatorAsset 側が持ち、handle は実行時に確保されるため保存しない。
		a_ar.Field("masterWeight",	_comp.masterWeight);
		a_ar.Field("yawLimitDeg",	_comp.yawLimitDeg);
		a_ar.Field("pitchLimitDeg",	_comp.pitchLimitDeg);
		a_ar.Field("followRate",	_comp.followRate);
		a_ar.Field("lagStiffness",	_comp.lagStiffness);
		a_ar.Field("lagDamping",	_comp.lagDamping);
		a_ar.Field("lagScale",		_comp.lagScale);
		a_ar.Field("lagLimitDeg",	_comp.lagLimitDeg);
		a_ar.Field("lagArmScale",	_comp.lagArmScale);
		a_ar.Field("lagLegScale",	_comp.lagLegScale);
		// 以下は後から足したもの(バイナリは順番読みなので末尾へ足す)
		a_ar.Field("airBlendRate",	_comp.airBlendRate);
		a_ar.Field("leanNormalDeg",	_comp.leanNormalDeg);
		a_ar.Field("leanBoostDeg",	_comp.leanBoostDeg);
		a_ar.Field("leanDashDeg",	_comp.leanDashDeg);
		a_ar.Field("leanFullSpeed",	_comp.leanFullSpeed);
		a_ar.Field("leanStiffness",	_comp.leanStiffness);
		a_ar.Field("leanDamping",	_comp.leanDamping);
	}

	static void Edit(CompEditContext& a_context)
	{
		using namespace Engine;
		App::Component::AdditivePoseComponent& _comp = Engine::EditorField::RefValue<App::Component::AdditivePoseComponent>(a_context.pData);

		Engine::EditorField::Field("MasterWeight", _comp.masterWeight, 0.01f, 0.0f, 1.0f);

		Engine::EditorField::Header("Aim");
		Engine::EditorField::Field("YawLimit(deg)", _comp.yawLimitDeg, 0.5f, 0.0f, 180.0f);
		Engine::EditorField::Field("PitchLimit(deg)", _comp.pitchLimitDeg, 0.5f, 0.0f, 90.0f);
		Engine::EditorField::Field("FollowRate", _comp.followRate, 0.1f, 0.0f);

		Engine::EditorField::Header("Lag");
		Engine::EditorField::Field("Stiffness", _comp.lagStiffness, 0.1f, 0.0f);
		Engine::EditorField::Field("Damping", _comp.lagDamping, 0.1f, 0.0f);
		Engine::EditorField::Field("Scale", _comp.lagScale, 0.001f, 0.0f);
		Engine::EditorField::Field("LagLimit(deg)", _comp.lagLimitDeg, 0.5f, 0.0f, 90.0f);
		Engine::EditorField::Field("ArmScale", _comp.lagArmScale, 0.01f, 0.0f);
		Engine::EditorField::Field("LegScale", _comp.lagLegScale, 0.01f, 0.0f);

		Engine::EditorField::Header("Air");
		Engine::EditorField::Field("BlendRate", _comp.airBlendRate, 0.1f, 0.0f);
		Engine::EditorField::Field("LeanNormal(deg)", _comp.leanNormalDeg, 0.5f, 0.0f, 90.0f);
		Engine::EditorField::Field("LeanBoost(deg)", _comp.leanBoostDeg, 0.5f, 0.0f, 90.0f);
		Engine::EditorField::Field("LeanDash(deg)", _comp.leanDashDeg, 0.5f, 0.0f, 90.0f);
		Engine::EditorField::Field("LeanFullSpeed", _comp.leanFullSpeed, 0.1f, 0.0f);
		Engine::EditorField::Field("LeanStiffness", _comp.leanStiffness, 0.1f, 0.0f);
		Engine::EditorField::Field("LeanDamping", _comp.leanDamping, 0.1f, 0.0f);
		Engine::EditorField::Value("AirBlend", "%.2f", _comp.airBlend);
		Engine::EditorField::Value("Lean(deg)", "x %.1f / z %.1f",
			DirectX::XMConvertToDegrees(_comp.bodyLeanAngle.x), DirectX::XMConvertToDegrees(_comp.bodyLeanAngle.z));

		Engine::EditorField::Header("Runtime");
		Engine::EditorField::HandleInfo(_comp.handle);

		// 解決済みボーンの確認(読み取り専用)
		if (a_context.pWorld)
		{
			auto& _pool = a_context.pWorld->RefResource<Engine::Pool::RangePool<App::InstanceResource::AdditiveBoneEntry>>();
			auto _entryVec = _pool.GetRange(_comp.handle);
			if (_entryVec.empty())
			{
				Engine::EditorField::HelpText("No resolved bones");
			}
			for (size_t _i = 0; _i < _entryVec.size(); ++_i)
			{
				const App::InstanceResource::AdditiveBoneEntry& _entry = _entryVec[_i];
				Engine::EditorField::Text("[%zu] node=%d share=%.2f ch=%s", _i, _entry.nodeIdx, _entry.share, Resource::ToString(_entry.channel));
			}
		}
	}
};
