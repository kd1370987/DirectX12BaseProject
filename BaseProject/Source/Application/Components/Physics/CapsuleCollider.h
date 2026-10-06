#pragma once

namespace App::Component
{
	// プレイヤー等に持たせるカプセルコライダー（テスト用）
	// 直立（ワールドY軸方向）のカプセルとして扱う。
	// 全長 = height + radius * 2 （height は両端の球中心間の距離＝線分の長さ）
	struct CapsuleColliderComponent
	{
		float radius = 0.5f;								// 半径
		float height = 1.0f;								// 端点間の距離（線分の長さ）
		Math::Vector3 offset = { 0.0f, 0.0f, 0.0f };	// エンティティ位置からの中心オフセット
	};
}

template<>
struct Engine::ECS::ComponentTraits<App::Component::CapsuleColliderComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		App::Component::CapsuleColliderComponent& _comp = Engine::EditorField::GetValue<App::Component::CapsuleColliderComponent>(a_pData);
		a_ar.Field("radius", _comp.radius);
		a_ar.Field("height", _comp.height);
		a_ar.Field("offset", _comp.offset);
	}

	static void Edit(CompEditContext& a_context)
	{
		App::Component::CapsuleColliderComponent& _comp = Engine::EditorField::GetValue<App::Component::CapsuleColliderComponent>(a_context.pData);
		Engine::EditorField::Field("Radius", _comp.radius, 0.05f, 0.0f, 100.0f);
		Engine::EditorField::Field("Height", _comp.height, 0.05f, 0.0f, 100.0f);
		Engine::EditorField::Field("Offset", _comp.offset, 0.05f);
	}
};
