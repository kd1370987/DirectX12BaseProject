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
		ImGui::Text("StartIndex : %d", static_cast<int>(_comp.skeletonPoseHandle.startIndex));
		ImGui::Text("Count : %d", static_cast<int>(_comp.skeletonPoseHandle.count));
		ImGui::Text("Generation : %d", static_cast<int>(_comp.skeletonPoseHandle.generation));
	}
};