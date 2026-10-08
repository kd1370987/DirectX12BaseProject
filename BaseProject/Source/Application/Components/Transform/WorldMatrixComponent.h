#pragma once

namespace App::Component
{
	struct WorldMatrixComponent
	{
		Math::Matrix worldMat= {};
		bool wasUpdatedThisFrame = true;
	};
}

template<>
struct Engine::ECS::ComponentTraits<App::Component::WorldMatrixComponent>
{
	static void Archive(Engine::Persistence::Archive& /*a_ar*/, void* a_pData)
	{
		App::Component::WorldMatrixComponent& _comp = Engine::EditorField::RefValue<App::Component::WorldMatrixComponent>(a_pData);
		_comp.worldMat = Math::Matrix::Identity();
		_comp.wasUpdatedThisFrame = false;
	}
	static void Edit(CompEditContext& a_context)
	{
		App::Component::WorldMatrixComponent& _comp = Engine::EditorField::RefValue<App::Component::WorldMatrixComponent>(a_context.pData);
		Engine::EditorField::Field("worldMat", _comp.worldMat);
	}
};