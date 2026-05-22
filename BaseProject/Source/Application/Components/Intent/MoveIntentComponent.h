#pragma once

// 復元時はGUIDからの復元

struct MoveIntentComponent
{
	DirectX::XMFLOAT3 value;


	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const MoveIntentComponent*>(a_ptr);
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<MoveIntentComponent*>(a_ptr);
	}

	static void Edit(void* a_data)
	{
		MoveIntentComponent& _comp = Engine::Editor::GetValue<MoveIntentComponent>(a_data);
	}
};