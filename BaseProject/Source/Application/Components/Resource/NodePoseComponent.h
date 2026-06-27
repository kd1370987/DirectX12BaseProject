#pragma once

struct NodePoseComponent
{
	Engine::RangeHandle<Engine::Resource::NodePoseMatrix> nodePoseHandle;

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const NodePoseComponent*>(a_ptr);
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<NodePoseComponent*>(a_ptr);
	}

	static void Edit(void* a_data)
	{
		NodePoseComponent& _comp = Engine::Editor::GetValue<NodePoseComponent>(a_data);
	}
};

template<>
struct Engine::ECS::ComponentTraits<NodePoseComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		NodePoseComponent& _comp = Engine::Editor::GetValue<NodePoseComponent>(a_pData);
	}

	static void Edit(void* a_pData)
	{
		NodePoseComponent& _comp = Engine::Editor::GetValue<NodePoseComponent>(a_pData);
		Editor::Helper::DrawHandle(_comp.nodePoseHandle);
	}
};