#pragma once

struct VelocityComponent
{
	Math::Vector3 value = { 0.0f, 0.0f, 0.0f };
};

template<>
struct Engine::ECS::ComponentTraits<VelocityComponent>
{
	static void Edit(CompEditContext& a_context)
	{
		VelocityComponent& _comp = Engine::Editor::GetValue<VelocityComponent>(a_context.pData);
		Engine::Editor::LabelText("", "%.2f, %.2f , %.2f", _comp.value.x, _comp.value.y, _comp.value.z);
		if (Engine::Editor::Button("Clear"))
		{
			_comp.value = {};
		}
	}
};
