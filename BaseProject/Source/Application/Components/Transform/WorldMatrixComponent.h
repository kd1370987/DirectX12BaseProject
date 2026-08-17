#pragma once

struct WorldMatrixComponent
{
	Math::Matrix worldMat= {};
	bool wasUpdatedThisFrame = true;
};

template<>
struct Engine::ECS::ComponentTraits<WorldMatrixComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		WorldMatrixComponent& _comp = Engine::Editor::GetValue<WorldMatrixComponent>(a_pData);
		_comp.worldMat = Math::Matrix::Identity();
		_comp.wasUpdatedThisFrame = false;
	}
	static void Edit(CompEditContext& a_context)
	{
		WorldMatrixComponent& _comp = Engine::Editor::GetValue<WorldMatrixComponent>(a_context.pData);
		Engine::Editor::EditorHelper::DrawMatrix("worldMat", _comp.worldMat);
	}
};