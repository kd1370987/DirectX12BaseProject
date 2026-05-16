#pragma once

struct StateComponent
{
	

	
	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const StateComponent*>(a_ptr);
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<StateComponent*>(a_ptr);
	}

	static void Edit(void* a_data)
	{
		StateComponent& _comp = Engine::Editor::GetValue<StateComponent>(a_data);
	}
};