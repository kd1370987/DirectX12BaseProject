#pragma once

// テスト用のボックス（AABB：ワールド軸に平行）コライダー
struct BoxColliderComponent
{
	DirectX::XMFLOAT3 extents = { 0.5f, 0.5f, 0.5f };	// 各軸の半分の長さ
	DirectX::XMFLOAT3 offset  = { 0.0f, 0.0f, 0.0f };	// エンティティ位置からの中心オフセット
};

template<>
struct Engine::ECS::ComponentTraits<BoxColliderComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		BoxColliderComponent& _comp = Engine::Editor::GetValue<BoxColliderComponent>(a_pData);
		a_ar.Field("extents", _comp.extents);
		a_ar.Field("offset", _comp.offset);
	}

	static void Edit(CompEditContext& a_context)
	{
		BoxColliderComponent& _comp = Engine::Editor::GetValue<BoxColliderComponent>(a_context.pData);
		ImGui::DragFloat3("Extents", &_comp.extents.x, 0.05f, 0.0f, 100.0f);
		ImGui::DragFloat3("Offset", &_comp.offset.x, 0.05f);
	}
};
