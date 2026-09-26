#pragma once

struct ProjMatComponent
{
	Math::Matrix projMat = {};     // 射影行列
	Math::Matrix projInvMat = {};  // 射影逆行列
};

template<>
struct Engine::ECS::ComponentTraits<ProjMatComponent>
{
	static void Edit(CompEditContext& a_context)
	{
		ProjMatComponent& _comp = Engine::Editor::GetValue<ProjMatComponent>(a_context.pData);
		Engine::Editor::Field("projMat", _comp.projMat);
		Engine::Editor::Field("projInvMat", _comp.projInvMat);
	}
};