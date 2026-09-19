#pragma once

struct PreviousWorldMatrixComponent
{
	Math::Matrix worldMat = {};
};

template<>
struct Engine::ECS::ComponentTraits<PreviousWorldMatrixComponent>
{
	static void Edit(CompEditContext& a_context)
	{
		PreviousWorldMatrixComponent& _comp = Engine::Editor::GetValue<PreviousWorldMatrixComponent>(a_context.pData);
		Engine::Editor::EditorHelper::DrawMatrix("prevWorldMat", _comp.worldMat);
	}
};