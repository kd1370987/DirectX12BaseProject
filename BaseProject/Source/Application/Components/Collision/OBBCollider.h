#pragma once

// テスト用のOBB（向きあり）コライダー
// 向きはエンティティの回転（LocalTransformComponent.quat）を使う
struct OBBColliderComponent
{
	DirectX::XMFLOAT3 extents = { 0.5f, 0.5f, 0.5f };	// 各軸の半分の長さ
	DirectX::XMFLOAT3 offset  = { 0.0f, 0.0f, 0.0f };	// エンティティ位置からの中心オフセット
};

template<>
struct Engine::ECS::ComponentTraits<OBBColliderComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		OBBColliderComponent& _comp = Engine::Editor::GetValue<OBBColliderComponent>(a_pData);
		a_ar.Field("extents", _comp.extents);
		a_ar.Field("offset", _comp.offset);
	}

	static void Edit(CompEditContext& a_context)
	{
		OBBColliderComponent& _comp = Engine::Editor::GetValue<OBBColliderComponent>(a_context.pData);
		ImGui::DragFloat3("Extents", &_comp.extents.x, 0.05f, 0.0f, 100.0f);
		ImGui::DragFloat3("Offset", &_comp.offset.x, 0.05f);
	}
};
