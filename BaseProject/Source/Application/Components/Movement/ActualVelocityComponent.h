#pragma once

namespace App::Component
{
	//==========================================================================================
	// ActualVelocityComponent
	//
	// 加減速を適用した実速度。実際に座標を進めるのはこの値(MovementIntegrationSystem)。
	//
	// ・書くのは MovementIntegrationSystem(目標速度へ加減速で寄せる)と、
	//   踏み込みの瞬間に直接入れる RobotBoostSystem / ChargeDashSystem。
	//   読むのは向き(SwarmLookSystem)・カメラ(TPSSystem)・追従(PlatoonFollowSystem)など。
	// ・以前は MovementComponent::velocity として、読むだけの設定(moveSpeed など)と
	//   同じ型に入っていた。設定を読むだけの側まで実速度の書き手とぶつかっていた。
	// ・MovementParamsComponent の必須コンポーネントなので、プレハブに書かなくても付く。
	//   毎フレーム計算される値なので保存しない。
	//==========================================================================================
	struct ActualVelocityComponent
	{
		Math::Vector3 value = { 0.0f, 0.0f, 0.0f };
	};
}

template<>
struct Engine::ECS::ComponentTraits<App::Component::ActualVelocityComponent>
{
	static void Edit(CompEditContext& a_context)
	{
		App::Component::ActualVelocityComponent& _comp = Engine::EditorField::GetValue<App::Component::ActualVelocityComponent>(a_context.pData);
		Engine::EditorField::Value("Velocity", "%.2f, %.2f , %.2f", _comp.value.x, _comp.value.y, _comp.value.z);
	}
};
