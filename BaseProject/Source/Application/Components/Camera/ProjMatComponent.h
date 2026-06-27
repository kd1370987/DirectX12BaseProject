#pragma once

struct ProjMatComponent
{
	DirectX::XMFLOAT4X4 projMat = {};     // 射影行列
	DirectX::XMFLOAT4X4 projInvMat = {};  // 射影逆行列
};

template<>
struct Engine::ECS::ComponentTraits<ProjMatComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		ProjMatComponent& _comp = Engine::Editor::GetValue<ProjMatComponent>(a_pData);
	}

	static void Edit(void* a_pData)
	{
		ProjMatComponent& _comp = Engine::Editor::GetValue<ProjMatComponent>(a_pData);
		ImGui::Text("projMat");
		Engine::Editor::Helper::DrawMatrix(_comp.projMat);
		ImGui::Text("projInvMat");
		Engine::Editor::Helper::DrawMatrix(_comp.projInvMat);
	}
};