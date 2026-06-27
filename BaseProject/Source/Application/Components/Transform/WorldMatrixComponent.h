#pragma once

struct WorldMatrixComponent
{
	DirectX::XMFLOAT4X4 worldMat= {};
	bool wasUpdatedThisFrame = true;
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