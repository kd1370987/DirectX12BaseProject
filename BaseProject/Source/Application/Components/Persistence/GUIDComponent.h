#pragma once
struct GUIDComponent
{
	//UUID guid = {};
	Engine::GUID guid = {};

	static void Serialize(const void* a_ptr,nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const GUIDComponent*>(a_ptr);
		a_json["guid"] = _comp->guid.String();
	}

	static void Deserialize(void* a_ptr,const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<GUIDComponent*>(a_ptr);
		_comp->guid.FromString(a_json["guid"].get<std::string>());
	}

	static void Edit(void* a_data)
	{
		GUIDComponent& _comp = Engine::Editor::GetValue<GUIDComponent>(a_data);
		ImGui::Text("%s",_comp.guid.String().c_str());
	}
};

template<>
struct Engine::ECS::ComponentTraits<GUIDComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		GUIDComponent& _comp = Engine::Editor::GetValue<GUIDComponent>(a_pData);
		a_ar.Field("guid", _comp.guid);
	}

	static void Edit(void* a_pData)
	{
		GUIDComponent& _comp = Engine::Editor::GetValue<GUIDComponent>(a_pData);
		ImGui::Text("%s", _comp.guid.String().c_str());
	}
};