#pragma once

struct PlayerLookAngleComponent
{
	float Yaw = 0.0f;
	float Pitch = 0.0f;

	float maxPitch = 80.0f;
};

template<>
struct Engine::ECS::ComponentTraits<PlayerLookAngleComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		PlayerLookAngleComponent& _comp = Engine::Editor::GetValue<PlayerLookAngleComponent>(a_pData);
		a_ar.Field("Yaw", _comp.Yaw);
		a_ar.Field("Pith", _comp.Pitch);
		a_ar.Field("maxPitch", _comp.maxPitch);
	}

	static void Edit(CompEditContext& a_context)
	{
		PlayerLookAngleComponent& _comp = Engine::Editor::GetValue<PlayerLookAngleComponent>(a_context.pData);
		ImGui::DragFloat("Yaw", &_comp.Yaw, 0.1f);
		ImGui::DragFloat("Pith", &_comp.Pitch, 0.1f);
		ImGui::Separator();
		ImGui::DragFloat("MaxPitch", &_comp.maxPitch, 0.1f);
	}
};