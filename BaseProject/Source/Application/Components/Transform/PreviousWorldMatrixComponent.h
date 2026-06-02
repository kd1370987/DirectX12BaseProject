#pragma once

struct PreviousWorldMatrixComponent
{
	DirectX::XMFLOAT4X4 worldMat = {};

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const PreviousWorldMatrixComponent*>(a_ptr);
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<PreviousWorldMatrixComponent*>(a_ptr);

	}

	static void Edit(void* a_data)
	{
		using namespace Engine;
		PreviousWorldMatrixComponent& _comp = Engine::Editor::GetValue<PreviousWorldMatrixComponent>(a_data);
		ImGui::Text("PreviousWorldMatrix");
		float* _m = (float*)_comp.worldMat.m;
		for (int _i = 0; _i < 4; ++_i)
		{
			ImGui::DragFloat4("##row", &_m[_i * 4]);
		}
	}
};