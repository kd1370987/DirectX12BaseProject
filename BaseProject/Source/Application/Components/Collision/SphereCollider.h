#pragma once

// 押し出しテスト用の球コライダー
struct SphereColliderComponent
{
	float radius = 0.5f;								// 半径
	DirectX::XMFLOAT3 offset = { 0.0f, 0.0f, 0.0f };	// エンティティ位置からの中心オフセット
};

template<>
struct Engine::ECS::ComponentTraits<SphereColliderComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		SphereColliderComponent& _comp = Engine::Editor::GetValue<SphereColliderComponent>(a_pData);
		a_ar.Field("radius", _comp.radius);
		a_ar.Field("offset", _comp.offset);
	}

	static void Edit(CompEditContext& a_context)
	{
		SphereColliderComponent& _comp = Engine::Editor::GetValue<SphereColliderComponent>(a_context.pData);
		ImGui::DragFloat("Radius", &_comp.radius, 0.05f, 0.0f, 100.0f);
		ImGui::DragFloat3("Offset", &_comp.offset.x, 0.05f);
	}
};
