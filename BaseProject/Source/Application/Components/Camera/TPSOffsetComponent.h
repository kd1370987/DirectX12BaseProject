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
		using namespace Engine;
		_comp->x = JSONHelper::GetValue<float>("x", a_json, 0);
		_comp->y = JSONHelper::GetValue<float>("y", a_json, 0);
		_comp->z = JSONHelper::GetValue<float>("z", a_json, 0);
	}

	static void Edit(void* a_data)
	{
		TPSOffsetComponent& _comp = Engine::Editor::GetValue<TPSOffsetComponent>(a_data);
		ImGui::DragFloat("x", &_comp.x);
		ImGui::DragFloat("y", &_comp.y);
		ImGui::DragFloat("z", &_comp.z);
	}

};