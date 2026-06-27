#pragma once

// 慣性コンポーネント

struct InertiaComponent
{
	float value = 0.0f;
};

template<>
struct Engine::ECS::ComponentTraits<InertiaComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		InertiaComponent& _comp = Engine::Editor::GetValue<InertiaComponent>(a_pData);
		a_ar.Field("Inertia", _comp.value);
	}

	static void Edit(void* a_pData)
	{
		InertiaComponent& _comp = Engine::Editor::GetValue<InertiaComponent>(a_pData);
		ImGui::DragFloat("Inertia", &_comp.value, 0.1f);
	}
};