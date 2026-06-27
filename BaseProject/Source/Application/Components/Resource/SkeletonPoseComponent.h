#pragma once

struct SkeletonPoseComponent
{
	Engine::RangeHandle<Engine::Resource::BoneMatrix> skeletonPoseHandle;
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