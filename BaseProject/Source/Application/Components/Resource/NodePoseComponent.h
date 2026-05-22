#pragma once

struct NodePoseComponent
{
	Engine::Storage::Range nodeRange;

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

		int _startIdx = (int)_comp.nodeRange.startIndex;
		int _rangeNum = (int)_comp.nodeRange.rangeSize;
		ImGui::Text("StartIndex : %d", _startIdx);
		ImGui::Text("RangeNum : %d", _rangeNum);
	}
};