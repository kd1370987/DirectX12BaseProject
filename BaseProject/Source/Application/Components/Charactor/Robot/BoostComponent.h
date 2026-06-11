#pragma once

struct BoostComponent
{
	// 推進力係数
	float boostPow = 0.0f;

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		using namespace Engine;
		auto* _comp = static_cast<const BoostComponent*>(a_ptr);
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		using namespace Engine;
		auto* _comp = static_cast<BoostComponent*>(a_ptr);
	}

	static void Edit(void* a_data)
	{
		using namespace Engine;
		BoostComponent& _comp = Engine::Editor::GetValue<BoostComponent>(a_data);
	}
};