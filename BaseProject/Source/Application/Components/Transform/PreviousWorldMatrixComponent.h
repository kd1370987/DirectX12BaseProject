#pragma once

namespace App::Component
{
	struct PreviousWorldMatrixComponent
	{
		Math::Matrix worldMat = {};
	};
}

template<>
struct Engine::ECS::ComponentTraits<App::Component::PreviousWorldMatrixComponent>
{
	static void Edit(CompEditContext& a_context)
	{
		App::Component::PreviousWorldMatrixComponent& _comp = Engine::EditorField::RefValue<App::Component::PreviousWorldMatrixComponent>(a_context.pData);
		Engine::EditorField::Field("prevWorldMat", _comp.worldMat);
	}
};