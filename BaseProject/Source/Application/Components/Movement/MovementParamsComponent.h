#pragma once

#include "DesiredVelocityComponent.h"
#include "ActualVelocityComponent.h"

namespace App::Component
{
	//==========================================================================================
	// MovementParamsComponent
	//
	// 「移動そのもの」の設定。保存される。システムは読むだけ。
	//
	//   moveSpeed    … 移動入力(MoveIntent)を速度へ変換するときの最大速度
	//   acceleration … 目標速度へ近づくときの加速度
	//   deceleration … 目標速度が今より遅いとき(離したとき/止まるとき)の減速度
	//
	// DesiredVelocityComponent は「目標速度」で、入力やブーストで 0 → 30 のように 1 フレームで
	// 飛ぶ。そこへ加速度/減速度で追従した結果が ActualVelocityComponent で、実際に座標を
	// 進めるのはこちら(MovementIntegrationSystem)。
	//
	// ・加減速がかかるのは水平(XZ)だけ。上下は重力/ジャンプ/ブーストの担当なので、
	//   目標速度をそのまま通す(加減速を挟むと落下や着地が鈍る)。
	// ・acceleration / deceleration が 0 以下なら「加減速なし」= 目標速度が即座に乗る。
	// ・以前の MovementComponent から実速度(ActualVelocityComponent)を分けた残り。
	//   目標速度と実速度は必須コンポーネントとして自動で付く。
	//==========================================================================================
	struct MovementParamsComponent
	{
		float moveSpeed    = 5.0f;		// 最大移動速度(units/sec)
		float acceleration = 30.0f;		// 加速度(units/sec^2)
		float deceleration = 30.0f;		// 減速度(units/sec^2)
	};
}

template<>
struct Engine::ECS::ComponentTraits<App::Component::MovementParamsComponent>
{
	// 加減速は目標速度を実速度へ寄せる処理なので、両方を必ず持たせる
	using Requires = Engine::ECS::RequireComponents<App::Component::DesiredVelocityComponent, App::Component::ActualVelocityComponent>;

	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		App::Component::MovementParamsComponent& _comp = Engine::EditorField::RefValue<App::Component::MovementParamsComponent>(a_pData);
		a_ar.Field("moveSpeed", _comp.moveSpeed);
		a_ar.Field("acceleration", _comp.acceleration);
		a_ar.Field("deceleration", _comp.deceleration);
	}

	static void Edit(CompEditContext& a_context)
	{
		App::Component::MovementParamsComponent& _comp = Engine::EditorField::RefValue<App::Component::MovementParamsComponent>(a_context.pData);
		Engine::EditorField::Field("MoveSpeed", _comp.moveSpeed, 0.1f, 0.0f, FLT_MAX);
		Engine::EditorField::Field("Acceleration", _comp.acceleration, 0.1f, 0.0f, FLT_MAX);
		Engine::EditorField::Field("Deceleration", _comp.deceleration, 0.1f, 0.0f, FLT_MAX);
	}
};
