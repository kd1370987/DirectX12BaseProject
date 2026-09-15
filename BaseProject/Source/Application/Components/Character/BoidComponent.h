#pragma once

struct BoidComponent
{
	float distanceLenge = 0.0f;

	Math::Vector3 targetPos;
	float pow = 0.0f;
};

template<>
struct Engine::ECS::ComponentTraits<BoidComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		BoidComponent& _comp = Engine::Editor::GetValue<BoidComponent>(a_pData);
		a_ar.Field("distanceLenge", _comp.distanceLenge);
		a_ar.Field("targetPos", _comp.targetPos);
		a_ar.Field("pow", _comp.pow);
	}

	static void Edit(CompEditContext& a_context)
	{
		BoidComponent& _comp = Engine::Editor::GetValue<BoidComponent>(a_context.pData);
		ImGui::DragFloat("DistanceLenge", &_comp.distanceLenge);
		ImGui::DragFloat3("targetPos",&_comp.targetPos.x);
		ImGui::DragFloat("pow",&_comp.pow);
	}
};