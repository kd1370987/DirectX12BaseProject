#pragma once

struct TPSLookAngleComponent
{
	float Pitch = 0.0f;
	float ClampPitch = 80.0f;
};

template<>
struct Engine::ECS::ComponentTraits<TPSLookAngleComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		TPSLookAngleComponent& _comp = Engine::Editor::GetValue<TPSLookAngleComponent>(a_pData);
		a_ar.Field("Pitch", _comp.Pitch);
		a_ar.Field("ClampPitch", _comp.ClampPitch);
	}

	static void Edit(void* a_pData)
	{
		TPSLookAngleComponent& _comp = Engine::Editor::GetValue<TPSLookAngleComponent>(a_pData);
		ImGui::DragFloat("Pitch", &_comp.Pitch);
		ImGui::DragFloat("CrampPitch", &_comp.ClampPitch);
	}
};