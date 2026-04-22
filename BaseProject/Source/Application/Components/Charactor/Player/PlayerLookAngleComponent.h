#pragma once

struct PlayerLookAngleComponent
{
	float Yaw = 0.0f;

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const PlayerLookAngleComponent*>(a_ptr);
		a_json["Yaw"] = _comp->Yaw;
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<PlayerLookAngleComponent*>(a_ptr);
		using namespace Engine;
		_comp->Yaw = a_json["Yaw"].get<float>();
		_comp->Yaw = JSONHelper::GetValue<float>("Yaw", a_json, 0);
	}

	static void Edit(void* a_data)
	{
		PlayerLookAngleComponent& _comp = Engine::Editor::GetValue<PlayerLookAngleComponent>(a_data);
		ImGui::DragFloat("Yaw", &_comp.Yaw, 0.1f);

	}
};