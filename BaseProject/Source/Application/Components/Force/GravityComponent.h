#pragma once

struct GravityComponent
{
	float scale = -1.0f;
};

template<>
struct Engine::ECS::ComponentTraits<GravityComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		GravityComponent& _comp = Engine::Editor::GetValue<GravityComponent>(a_pData);
		a_ar.Field("scale", _comp.scale);
	}

	static void Edit(void* a_pData)
	{
		GravityComponent& _comp = Engine::Editor::GetValue<GravityComponent>(a_pData);
		ImGui::DragFloat("GravityScale", &_comp.scale, 0.1f);
	}
};