#pragma once

struct BoidComponent
{
	float distanceLenge = 0.0f;
};

template<>
struct Engine::ECS::ComponentTraits<BoidComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		BoidComponent& _comp = Engine::Editor::GetValue<BoidComponent>(a_pData);
		a_ar.Field("distanceLenge", _comp.distanceLenge);
	}

	static void Edit(CompEditContext& a_context)
	{
		BoidComponent& _comp = Engine::Editor::GetValue<BoidComponent>(a_context.pData);
		ImGui::DragFloat("DistanceLenge", &_comp.distanceLenge);
	}
};