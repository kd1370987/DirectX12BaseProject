#pragma once

struct SkeletonPoseComponent
{
	DirectX::XMFLOAT4X4 palette[300];

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const SkeletonPoseComponent*>(a_ptr);
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<SkeletonPoseComponent*>(a_ptr);
	}

	static void Edit(void* a_data)
	{
		using namespace Engine;
		SkeletonPoseComponent& _comp = Engine::Editor::GetValue<SkeletonPoseComponent>(a_data);
	}
};