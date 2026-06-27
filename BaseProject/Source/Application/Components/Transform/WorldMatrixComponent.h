#pragma once

struct WorldMatrixComponent
{
	DirectX::XMFLOAT4X4 worldMat= {};

	bool wasUpdatedThisFrame = true;

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const WorldMatrixComponent*>(a_ptr);
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<WorldMatrixComponent*>(a_ptr);
		
	}

	static void Edit(void* a_data)
	{
		using namespace Engine;
		WorldMatrixComponent& _comp = Engine::Editor::GetValue<WorldMatrixComponent>(a_data);
		ImGui::Text("WorldMatrix");
		float* _m = (float*)_comp.worldMat.m;
		for (int _i = 0; _i < 4; ++_i)
		{
			ImGui::DragFloat4("##row", &_m[_i * 4]);
		}
	}
};

template<>
struct Engine::ECS::ComponentTraits<WorldMatrixComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		WorldMatrixComponent& _comp = Engine::Editor::GetValue<WorldMatrixComponent>(a_pData);
		_comp.worldMat = DXSM::Matrix::Identity;
		_comp.wasUpdatedThisFrame = false;
	}
	static void Edit(void* a_pData)
	{
		WorldMatrixComponent& _comp = Engine::Editor::GetValue<WorldMatrixComponent>(a_pData);
		Engine::Editor::Helper::DrawMatrix(_comp.worldMat);
	}
};