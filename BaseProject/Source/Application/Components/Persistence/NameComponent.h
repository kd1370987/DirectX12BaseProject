#pragma once
struct NameComponent
{
	char name[64] = "Unknown";

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const NameComponent*>(a_ptr);
		a_json["name"] = _comp->name;
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		NameComponent& _comp = Engine::Editor::GetValue<NameComponent>(a_ptr);
		//_comp.name = a_json["name"].get<std::string>().c_str();
	}

	static void Edit(void* a_data)
	{
		NameComponent& _comp = Engine::Editor::GetValue<NameComponent>(a_data);
		ImGui::InputText("##Name", _comp.name, 64);
		ImGui::SameLine();
		ImGui::Text("Name");
	}
};