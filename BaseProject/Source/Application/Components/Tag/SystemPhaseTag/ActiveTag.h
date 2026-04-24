
#pragma once

struct ActiveTag {

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{}

	static void Edit(void* a_data)
	{}
};