#pragma once

struct TPSOffsetComponent
{
	float x = 0, y = 0, z = 0;

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const TPSOffsetComponent*>(a_ptr);
		a_json["x"] = _comp->x;
		a_json["y"] = _comp->y;
		a_json["z"] = _comp->z;
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<TPSOffsetComponent*>(a_ptr);
		_comp->x = a_json.at("x");
		_comp->y = a_json.at("y");
		_comp->z = a_json.at("z");
	}

	static void Edit(void* a_data)
	{
		TPSOffsetComponent& _comp = Engine::Editor::GetValue<TPSOffsetComponent>(a_data);
		ImGui::DragFloat("x", &_comp.x);
		ImGui::DragFloat("y", &_comp.y);
		ImGui::DragFloat("z", &_comp.z);
	}

};