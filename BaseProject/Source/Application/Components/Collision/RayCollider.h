#pragma once

struct RayColliderComponent
{
	float length = 0.0f;
	DirectX::XMFLOAT3 dir = { 0.0f,0.0f,0.0f };
	DirectX::XMFLOAT3 pos = { 0.0f,0.0f,0.0f };
};

template<>
struct Engine::ECS::ComponentTraits<RayColliderComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		RayColliderComponent& _comp = Engine::Editor::GetValue<RayColliderComponent>(a_pData);
		a_ar.Field("length", _comp.length);
		a_ar.Field("dir", _comp.dir);
		a_ar.Field("pos", _comp.pos);
	}

	static void Edit(void* a_pData)
	{
		RayColliderComponent& _comp = Engine::Editor::GetValue<RayColliderComponent>(a_pData);
		ImGui::DragFloat("Length", &_comp.length, 0.1f);
		ImGui::DragFloat3("Dir", &_comp.dir.x, 0.1f);
		ImGui::DragFloat3("Pos", &_comp.pos.x, 0.1f);
	}
};