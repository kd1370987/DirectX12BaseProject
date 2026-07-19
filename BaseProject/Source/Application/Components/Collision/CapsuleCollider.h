#pragma once

// プレイヤー等に持たせるカプセルコライダー（テスト用）
// 直立（ワールドY軸方向）のカプセルとして扱う。
// 全長 = height + radius * 2 （height は両端の球中心間の距離＝線分の長さ）
struct CapsuleColliderComponent
{
	float radius = 0.5f;								// 半径
	float height = 1.0f;								// 端点間の距離（線分の長さ）
	DirectX::XMFLOAT3 offset = { 0.0f, 0.0f, 0.0f };	// エンティティ位置からの中心オフセット
};

template<>
struct Engine::ECS::ComponentTraits<CapsuleColliderComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		CapsuleColliderComponent& _comp = Engine::Editor::GetValue<CapsuleColliderComponent>(a_pData);
		a_ar.Field("radius", _comp.radius);
		a_ar.Field("height", _comp.height);
		a_ar.Field("offset", _comp.offset);
	}

	static void Edit(CompEditContext& a_context)
	{
		CapsuleColliderComponent& _comp = Engine::Editor::GetValue<CapsuleColliderComponent>(a_context.pData);
		ImGui::DragFloat("Radius", &_comp.radius, 0.05f, 0.0f, 100.0f);
		ImGui::DragFloat("Height", &_comp.height, 0.05f, 0.0f, 100.0f);
		ImGui::DragFloat3("Offset", &_comp.offset.x, 0.05f);
	}
};
