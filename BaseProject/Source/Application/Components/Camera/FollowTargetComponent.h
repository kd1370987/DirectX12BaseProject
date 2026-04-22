#pragma once



struct FollowTargetComponent
{
	Engine::ECS::Entity target = Engine::ECS::Limits::INVALID_ENTITY;

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const FollowTargetComponent*>(a_ptr);
		// 初期化時後の最初のアップデートが呼ばれる前に情報からふくげんするから個々では市内
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		using namespace Engine;
		auto* _comp = static_cast<FollowTargetComponent*>(a_ptr);
	}

	static void Edit(void* a_data)
	{
		FollowTargetComponent& _comp = Engine::Editor::GetValue<FollowTargetComponent>(a_data);
		ImGui::InputScalar("TargetEntity", ImGuiDataType_U64, a_data);
	}
};