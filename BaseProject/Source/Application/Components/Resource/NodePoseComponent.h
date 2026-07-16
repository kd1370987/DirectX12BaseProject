#pragma once

struct NodePoseComponent
{
	Engine::RangeHandle<Engine::Resource::NodePoseMatrix> nodePoseHandle;
};

template<>
struct Engine::ECS::ComponentTraits<NodePoseComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		NodePoseComponent& _comp = Engine::Editor::GetValue<NodePoseComponent>(a_pData);
	}

	static void Edit(CompEditContext& a_context)
	{
		NodePoseComponent& _comp = Engine::Editor::GetValue<NodePoseComponent>(a_context.pData);
		Editor::Helper::DrawHandle(_comp.nodePoseHandle);
	}
};