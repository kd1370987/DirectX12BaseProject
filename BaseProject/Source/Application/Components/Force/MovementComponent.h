#pragma once

//==========================================================================================
// MovementComponent
//
// 「移動そのもの」の設定と実速度を持つ。旧 InertiaComponent の置き換え。
//
//   moveSpeed    … 移動入力(MoveIntent)を速度へ変換するときの最大速度
//   acceleration … 目標速度へ近づくときの加速度
//   deceleration … 目標速度が今より遅いとき(離したとき/止まるとき)の減速度
//
// VelocityComponent は「目標速度」で、入力やブーストで 0 → 30 のように 1 フレームで
// 飛ぶ。そこへ加速度/減速度で追従した結果がこの velocity で、実際に座標を進めるのは
// こちら(MovementIntegrationSystem)。旧 InertiaComponent の「時定数で指数的に追従」を
// 加速度・減速度に置き換えたもの。
//
// ・加減速がかかるのは水平(XZ)だけ。上下は重力/ジャンプ/ブーストの担当なので、
//   目標速度をそのまま通す(加減速を挟むと落下や着地が鈍る)。
// ・acceleration / deceleration が 0 以下なら「加減速なし」= 目標速度が即座に乗る。
//==========================================================================================
struct MovementComponent
{
	// ---- 設定(保存される) ----
	float moveSpeed    = 5.0f;		// 最大移動速度(units/sec)
	float acceleration = 30.0f;		// 加速度(units/sec^2)
	float deceleration = 30.0f;		// 減速度(units/sec^2)

	// ---- ランタイム ----
	DirectX::XMFLOAT3 velocity = { 0.0f, 0.0f, 0.0f };	// 加減速を適用した実速度
};

template<>
struct Engine::ECS::ComponentTraits<MovementComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		MovementComponent& _comp = Engine::Editor::GetValue<MovementComponent>(a_pData);
		a_ar.Field("moveSpeed", _comp.moveSpeed);
		a_ar.Field("acceleration", _comp.acceleration);
		a_ar.Field("deceleration", _comp.deceleration);
	}

	static void Edit(CompEditContext& a_context)
	{
		MovementComponent& _comp = Engine::Editor::GetValue<MovementComponent>(a_context.pData);
		ImGui::DragFloat("MoveSpeed", &_comp.moveSpeed, 0.1f, 0.0f, FLT_MAX);
		ImGui::DragFloat("Acceleration", &_comp.acceleration, 0.1f, 0.0f, FLT_MAX);
		ImGui::DragFloat("Deceleration", &_comp.deceleration, 0.1f, 0.0f, FLT_MAX);
		ImGui::LabelText("Velocity", "%.2f, %.2f , %.2f", _comp.velocity.x, _comp.velocity.y, _comp.velocity.z);
	}
};
