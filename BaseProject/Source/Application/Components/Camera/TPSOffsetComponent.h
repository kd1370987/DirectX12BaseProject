#pragma once

struct TPSOffsetComponent
{
	float x = 0, y = 0, z = 0;
};

template<>
struct Engine::ECS::ComponentTraits<TPSOffsetComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData) 
	{
		TPSOffsetComponent& _comp = Engine::Editor::GetValue<TPSOffsetComponent>(a_pData);
		a_ar.Field("x", _comp.x);
		a_ar.Field("y", _comp.y);
		a_ar.Field("z", _comp.z);
	}

	static void Edit(CompEditContext& a_context)
	{
		TPSOffsetComponent& _comp = Engine::Editor::GetValue<TPSOffsetComponent>(a_context.pData);
		ImGui::DragFloat("x", &_comp.x);
		ImGui::DragFloat("y", &_comp.y);
		ImGui::DragFloat("z", &_comp.z);
	}
};