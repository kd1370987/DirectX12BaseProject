#pragma once

struct SkeletonPoseComponent
{
	Engine::RangeHandle<Engine::Resource::BoneMatrix> skeletonPoseHandle;

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


template<>
struct Engine::ECS::ComponentTraits<SkeletonPoseComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		SkeletonPoseComponent& _comp = Engine::Editor::GetValue<SkeletonPoseComponent>(a_pData);
	
	}

	static void Edit(void* a_pData)
	{
		using namespace Engine;
		SkeletonPoseComponent& _comp = Engine::Editor::GetValue<SkeletonPoseComponent>(a_pData);
	}
};