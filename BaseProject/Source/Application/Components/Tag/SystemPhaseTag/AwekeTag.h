
#pragma once

struct AwekeTag {

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{}

	static void Edit(void* a_data)
	{}
};
template<>
struct Engine::ECS::ComponentTraits<AwekeTag>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData) {}
	static void Edit(void* a_pData) {}
};