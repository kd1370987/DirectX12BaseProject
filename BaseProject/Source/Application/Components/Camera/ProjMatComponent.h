#pragma once

struct ProjMatComponent
{
	Math::Matrix projMat = {};     // 射影行列
	Math::Matrix projInvMat = {};  // 射影逆行列
};

template<>
struct Engine::ECS::ComponentTraits<ProjMatComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		ProjMatComponent& _comp = Engine::Editor::GetValue<ProjMatComponent>(a_pData);
	}

	static void Edit(CompEditContext& a_context)
	{
		ProjMatComponent& _comp = Engine::Editor::GetValue<ProjMatComponent>(a_context.pData);
		Engine::Editor::EditorHelper::DrawMatrix("projMat", _comp.projMat);
		Engine::Editor::EditorHelper::DrawMatrix("projInvMat", _comp.projInvMat);
	}
};