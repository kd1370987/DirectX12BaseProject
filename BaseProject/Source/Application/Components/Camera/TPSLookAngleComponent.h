#pragma once

struct TPSLookAngleComponent
{
	float Pitch = 0.0f;
	float ClampPitch = 80.0f;

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const TPSLookAngleComponent*>(a_ptr);
		a_json["Pitch"] = _comp->Pitch;
		a_json["ClampPitch"] = _comp->ClampPitch;
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<TPSLookAngleComponent*>(a_ptr);
		_comp->Pitch = a_json.at("Pitch");
		_comp->ClampPitch = a_json.at("ClampPtich");
	}

	static void Edit(void* a_data)
	{
		TPSLookAngleComponent& _comp = Engine::Editor::GetValue<TPSLookAngleComponent>(a_data);
		ImGui::DragFloat("Pitch", &_comp.Pitch);
		ImGui::DragFloat("CrampPitch", &_comp.ClampPitch);
	}

};
