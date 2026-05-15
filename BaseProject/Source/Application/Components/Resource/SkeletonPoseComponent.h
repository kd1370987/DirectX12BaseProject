#pragma once

struct SkeletonPoseComponent
{
	DirectX::XMFLOAT4X4 palette[300];

	Engine::Storage::Range boneRange;

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const SkeletonPoseComponent*>(a_ptr);
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<SkeletonPoseComponent*>(a_ptr);
	}

	static void Edit(void* a_data)
	{
		using namespace Engine;
		SkeletonPoseComponent& _comp = Engine::Editor::GetValue<SkeletonPoseComponent>(a_data);

		int _startIdx = (int)_comp.boneRange.startIndex;
		int _rangeNum = (int)_comp.boneRange.rangeSize;
		ImGui::Text("StartIndex : %d", _startIdx);
		ImGui::Text("RangeNum : %d", _rangeNum);
	}
};