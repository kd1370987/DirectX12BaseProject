#pragma once

struct SkeletonPoseComponent
{
	Engine::RangeHandle<Engine::Resource::BoneMatrix> skeletonPoseHandle;
};


template<>
struct Engine::ECS::ComponentTraits<SkeletonPoseComponent>
{
	static void Edit(CompEditContext& a_context)
	{
		using namespace Engine;
		SkeletonPoseComponent& _comp = Engine::Editor::GetValue<SkeletonPoseComponent>(a_context.pData);
		Editor::EditorHelper::DrawHandle(_comp.skeletonPoseHandle);
	}
};