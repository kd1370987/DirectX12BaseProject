#pragma once

struct PreviousWorldMatrixComponent
{
	Math::Matrix worldMat = {};
};

template<>
struct Engine::ECS::ComponentTraits<PreviousWorldMatrixComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		PreviousWorldMatrixComponent& _comp = Engine::Editor::GetValue<PreviousWorldMatrixComponent>(a_pData);
		_comp.worldMat = Math::Matrix::Identity();
	}
	static void Edit(CompEditContext& a_context)
	{
		PreviousWorldMatrixComponent& _comp = Engine::Editor::GetValue<PreviousWorldMatrixComponent>(a_context.pData);
		Engine::Editor::EditorHelper::DrawMatrix("prevWorldMat", _comp.worldMat);
	}
};