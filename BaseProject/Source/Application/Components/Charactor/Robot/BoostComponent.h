#pragma once

struct BoostComponent
{
	// エネルギーマックス容量
	float maxEnergy = 0.0f;

	// 現在のエネルギー残量
	float currentEnergy = 0.0f;

	// ブーストに使うデータ
	float boostPow = 0.0f;		// 推進力係数
	float boostEnergy = 0.0f;	// ブースト一回に使うエネルギー


	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		using namespace Engine;
		auto* _comp = static_cast<const BoostComponent*>(a_ptr);
		a_json["maxEnergy"] = _comp->maxEnergy;
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