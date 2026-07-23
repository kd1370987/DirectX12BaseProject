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

	static void Edit(CompEditContext& a_context)
	{
		using namespace Engine;
		SkeletonPoseComponent& _comp = Engine::Editor::GetValue<SkeletonPoseComponent>(a_context.pData);
		Editor::Helper::DrawHandle(_comp.skeletonPoseHandle);
	}
};