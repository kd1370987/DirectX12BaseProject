#pragma once

struct PlayerLookAngleComponent
{
	float Yaw = 0.0f;
	float Pitch = 0.0f;

	float maxPitch = 80.0f;

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const PlayerLookAngleComponent*>(a_ptr);
		a_json["Yaw"] = _comp->Yaw;
		a_json["maxPitch"] = _comp->maxPitch;
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<PlayerLookAngleComponent*>(a_ptr);
		using namespace Engine;
		_comp->Yaw = a_json["Yaw"].get<float>();
		_comp->Yaw = JSONHelper::GetValue<float>("Yaw", a_json, 0);
		_comp->maxPitch = JSONHelper::GetValue<float>("maxPitch",a_json,90);
	}

	static void Edit(void* a_data)
	{
		PlayerLookAngleComponent& _comp = Engine::Editor::GetValue<PlayerLookAngleComponent>(a_data);
		ImGui::DragFloat("Yaw", &_comp.Yaw, 0.1f);
		ImGui::DragFloat("Pith", &_comp.Yaw, 0.1f);
		ImGui::Separator();

		ImGui::DragFloat("MaxPitch",&_comp.maxPitch,0.1f);
	}
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

	static void Edit(void* a_pData)
	{
		PlayerLookAngleComponent& _comp = Engine::Editor::GetValue<PlayerLookAngleComponent>(a_pData);
		ImGui::DragFloat("Yaw", &_comp.Yaw, 0.1f);
		ImGui::DragFloat("Pith", &_comp.Pitch, 0.1f);
		ImGui::Separator();
		ImGui::DragFloat("MaxPitch", &_comp.maxPitch, 0.1f);
	}
};