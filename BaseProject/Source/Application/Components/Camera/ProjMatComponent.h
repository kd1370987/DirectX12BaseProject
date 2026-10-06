#pragma once

namespace App::Component
{
	struct ProjMatComponent
	{
		Math::Matrix projMat = {};     // 射影行列
		Math::Matrix projInvMat = {};  // 射影逆行列
	};
}

template<>
struct Engine::ECS::ComponentTraits<App::Component::ProjMatComponent>
{
	static void Edit(CompEditContext& a_context)
	{
		App::Component::ProjMatComponent& _comp = Engine::EditorField::GetValue<App::Component::ProjMatComponent>(a_context.pData);
		Engine::EditorField::Field("projMat", _comp.projMat);
		Engine::EditorField::Field("projInvMat", _comp.projInvMat);
	}
};