#pragma once

struct NodePoseComponent
{
	Engine::RangeHandle<Engine::Resource::NodePoseMatrix> nodePoseHandle;
};

template<>
struct Engine::ECS::ComponentTraits<NodePoseComponent>
{
	static void Edit(CompEditContext& a_context)
	{
		NodePoseComponent& _comp = Engine::Editor::GetValue<NodePoseComponent>(a_context.pData);
		Editor::EditorHelper::DrawHandle(_comp.nodePoseHandle);
	}
};