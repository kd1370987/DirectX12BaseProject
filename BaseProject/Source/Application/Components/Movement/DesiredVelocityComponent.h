#pragma once

namespace App::Component
{
	//==========================================================================================
	// DesiredVelocityComponent
	//
	// 目標速度。移動・AI・重力・ブーストなど、動かしたい側が書く。
	//
	// ・MovementParamsComponent を持つものは、積分(MovementIntegrationSystem)が
	//   加減速を掛けて実速度(ActualVelocityComponent)へ寄せ、その実速度で進む。
	// ・持たないもの(弾・ボイドなど)は加減速が無いので、この値がそのまま実速度として
	//   積分される(PositionIntegrationSystem)。
	// ・以前は VelocityComponent という名前で、持ち主によって意味が変わっていた。
	//==========================================================================================
	struct DesiredVelocityComponent
	{
		Math::Vector3 value = { 0.0f, 0.0f, 0.0f };
	};
}

template<>
struct Engine::ECS::ComponentTraits<App::Component::DesiredVelocityComponent>
{
	static void Edit(CompEditContext& a_context)
	{
		App::Component::DesiredVelocityComponent& _comp = Engine::EditorField::GetValue<App::Component::DesiredVelocityComponent>(a_context.pData);
		Engine::EditorField::Value("", "%.2f, %.2f , %.2f", _comp.value.x, _comp.value.y, _comp.value.z);
		if (Engine::EditorField::Button("Clear"))
		{
			_comp.value = {};
		}
	}
};
