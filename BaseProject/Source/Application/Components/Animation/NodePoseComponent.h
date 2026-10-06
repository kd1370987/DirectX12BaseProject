#pragma once

namespace App::Component
{
	struct NodePoseComponent
	{
		Engine::RangeHandle<Engine::Resource::NodePoseMatrix> nodePoseHandle;
	};
}

template<>
struct Engine::ECS::ComponentTraits<App::Component::NodePoseComponent>
{
	static void Edit(CompEditContext& a_context)
	{
		App::Component::NodePoseComponent& _comp = Engine::EditorField::RefValue<App::Component::NodePoseComponent>(a_context.pData);
		Engine::EditorField::HandleInfo(_comp.nodePoseHandle);
	}
};