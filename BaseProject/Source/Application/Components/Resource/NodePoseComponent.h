#pragma once

constexpr UINT MAX_NODEINDEX = 100;

struct NodePoseComponent
{
	DirectX::XMFLOAT4X4 local[MAX_NODEINDEX];
	DirectX::XMFLOAT4X4 world[MAX_NODEINDEX];
	uint16_t nodeCount;

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
		ImGui::Text("NodeCount : %f",&_comp.nodeCount);

	}
};