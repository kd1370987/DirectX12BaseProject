#pragma once
struct GUIDComponent
{
	UUID guid = {};

	static void Serialize(const void* a_ptr,nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const GUIDComponent*>(a_ptr);
		a_json["guid"] = Engine::GUID::ToString(_comp->guid);
	}

	static void Deserialize(void* a_ptr,const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<GUIDComponent*>(a_ptr);
		_comp->guid = Engine::GUID::FromString(a_json["guid"].get<std::string>());
	}

	static void Edit(void* a_data)
	{
		GUIDComponent& _comp = Engine::Editor::GetValue<GUIDComponent>(a_data);
		ImGui::Text("%s", Engine::GUID::ToString(_comp.guid).c_str());
	}
};