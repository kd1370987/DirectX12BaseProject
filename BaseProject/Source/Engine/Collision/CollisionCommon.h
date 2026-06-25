#pragma once
namespace Engine::Collision
{
	// コリジョンワールドに登録するインスタンス
	struct CollisionInstance
	{
		ECS::Entity entity = ECS::Limits::INVALID_ENTITY;	// エンティティID
		Handle<Resource::Model> modelHandle = {};			// モデルのハンドル
		DirectX::XMFLOAT4X4 worldMat = {};					// ワールド行列

		// モデル全体のAABBをワールド空間に変換したボックス
		DirectX::BoundingBox worldAABB = {};
		uint32_t layer = 0;
	};

	// ヒットした際に帰ってくる情報
	struct Result
	{
		ECS::Entity hitEntity = ECS::Limits::INVALID_ENTITY;
		DirectX::XMFLOAT3 hitPos = { 0.0f,0.0f,0.0f };
		DirectX::XMFLOAT3 hitNormal = { 0.0f,0.0f,0.0f };
		float hitDistance = 0.0f;
		bool isHit = false;
	};

	// レイ判定時に渡す情報
	struct RayInfo
	{
		DirectX::XMFLOAT3 origin = { 0.0f,0.0f,0.0f };
		DirectX::XMFLOAT3 direction = { 0.0f,0.0f,1.0f };
		float maxDistance = 1000.0f;
	};

	// 球判定時に渡す情報

}