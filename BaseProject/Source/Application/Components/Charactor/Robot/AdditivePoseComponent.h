#pragma once

#include "Engine/ECS/World/World.h"
#include "Application/InstanceResource/AdditiveBoneEntry.h"

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
	Engine::RangeHandle<AdditiveBoneEntry> handle = {};

	// --- 調整値 ---
	float masterWeight	= 1.0f;		// エンティティ全体の効き(0で完全無効)
	float yawLimitDeg	= 60.0f;	// 上半身の可動域(左右)
	float pitchLimitDeg	= 35.0f;	// 上半身の可動域(上下)
	float followRate	= 12.0f;	// 照準追従のSlerp速度
	float lagStiffness	= 20.0f;	// 引っ張られのバネ定数
	float lagDamping	= 8.0f;		// 引っ張られの減衰
	float lagScale		= 0.02f;	// 加速度→角度の変換係数
	float lagLimitDeg	= 25.0f;	// 引っ張られの最大角
	float lagArmScale	= 1.0f;		// LagArm チャンネルの倍率
	float lagLegScale	= 0.7f;		// LagLeg チャンネルの倍率

	// --- 実行時 ---
	// currentAimQuat は必ず単位クォータニオンで初期化すること。
	// ゼロクォータニオンを XMMatrixRotationQuaternion に渡すとスケール0の行列になり、
	// メッシュが原点に潰れる。
	DirectX::XMFLOAT4 currentAimQuat	= { 0.0f, 0.0f, 0.0f, 1.0f };	// 現在の上半身回転(補間後)
	DirectX::XMFLOAT3 lagAngle			= { 0.0f, 0.0f, 0.0f };			// バネの現在値(ラジアン)
	DirectX::XMFLOAT3 lagVelocity		= { 0.0f, 0.0f, 0.0f };			// バネの速度
	DirectX::XMFLOAT3 prevVelocity		= { 0.0f, 0.0f, 0.0f };			// 加速度算出用
	bool			  isPrevVelocityValid = false;						// 初回フレームの加速度暴れ防止
};

template<>
struct Engine::ECS::ComponentTraits<AdditivePoseComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		AdditivePoseComponent& _comp = Engine::Editor::GetValue<AdditivePoseComponent>(a_pData);

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
	}

	static void Edit(CompEditContext& a_context)
	{
		using namespace Engine;
		AdditivePoseComponent& _comp = Engine::Editor::GetValue<AdditivePoseComponent>(a_context.pData);

		ImGui::DragFloat("MasterWeight", &_comp.masterWeight, 0.01f, 0.0f, 1.0f);

		ImGui::SeparatorText("Aim");
		ImGui::DragFloat("YawLimit(deg)",	&_comp.yawLimitDeg,		0.5f, 0.0f, 180.0f);
		ImGui::DragFloat("PitchLimit(deg)",	&_comp.pitchLimitDeg,	0.5f, 0.0f, 90.0f);
		ImGui::DragFloat("FollowRate",		&_comp.followRate,		0.1f, 0.0f);

		ImGui::SeparatorText("Lag");
		ImGui::DragFloat("Stiffness",		&_comp.lagStiffness,	0.1f, 0.0f);
		ImGui::DragFloat("Damping",			&_comp.lagDamping,		0.1f, 0.0f);
		ImGui::DragFloat("Scale",			&_comp.lagScale,		0.001f, 0.0f);
		ImGui::DragFloat("LagLimit(deg)",	&_comp.lagLimitDeg,		0.5f, 0.0f, 90.0f);
		ImGui::DragFloat("ArmScale",		&_comp.lagArmScale,		0.01f, 0.0f);
		ImGui::DragFloat("LegScale",		&_comp.lagLegScale,		0.01f, 0.0f);

		ImGui::SeparatorText("Runtime");
		Editor::Helper::DrawHandle(_comp.handle);

		// 解決済みボーンの確認(読み取り専用)
		if (a_context.pWorld)
		{
			auto& _pool = a_context.pWorld->GetResource<Engine::Pool::RangePool<AdditiveBoneEntry>>();
			auto _entryVec = _pool.GetRange(_comp.handle);
			if (_entryVec.empty())
			{
				ImGui::TextDisabled("No resolved bones");
			}
			for (size_t _i = 0; _i < _entryVec.size(); ++_i)
			{
				const AdditiveBoneEntry& _entry = _entryVec[_i];
				ImGui::Text("[%zu] node=%d share=%.2f ch=%s",
					_i, _entry.nodeIdx, _entry.share,
					Resource::ToString(_entry.channel));
			}
		}
	}
};
