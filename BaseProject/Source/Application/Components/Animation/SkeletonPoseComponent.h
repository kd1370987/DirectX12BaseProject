#pragma once

namespace App::Component
{
	struct SkeletonPoseComponent
	{
		Engine::RangeHandle<Engine::Resource::BoneMatrix> skeletonPoseHandle;
	};
}

template<>
struct Engine::ECS::ComponentTraits<App::Component::SkeletonPoseComponent>
{
	static void Edit(CompEditContext& a_context)
	{
		using namespace Engine;
		App::Component::SkeletonPoseComponent& _comp = Engine::EditorField::GetValue<App::Component::SkeletonPoseComponent>(a_context.pData);
		Engine::EditorField::HandleInfo(_comp.skeletonPoseHandle);
	}
};