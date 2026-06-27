#pragma once

struct PreviousWorldMatrixComponent
{
	DirectX::XMFLOAT4X4 worldMat = {};
};

template<>
struct Engine::ECS::ComponentTraits<PreviousWorldMatrixComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		PreviousWorldMatrixComponent& _comp = Engine::Editor::GetValue<PreviousWorldMatrixComponent>(a_pData);
		_comp.worldMat = DXSM::Matrix::Identity;
	}
	static void Edit(void* a_pData)
	{
		PreviousWorldMatrixComponent& _comp = Engine::Editor::GetValue<PreviousWorldMatrixComponent>(a_pData);
		Engine::Editor::Helper::DrawMatrix(_comp.worldMat);
	}
};