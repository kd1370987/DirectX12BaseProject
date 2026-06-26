#pragma once
namespace Engine::Collision
{
	// 形状
	enum class EShapeType
	{
		Sphere,
		Box,
		Capsule,
		Mesh			// BVHトラバースが入るため少し重い
	};

	// 形状のデータをまとめる
	struct ColliderShape
	{
		EShapeType type;

		union
		{
			struct { float radius; } sphere;					// 球体
			struct { DirectX::XMFLOAT3 extents; } box;			// ボックス
			struct { float radius; float height; } capsule;		// カプセル
			Handle<Resource::Model> modelHandle;				// メッシュの場合はモデルのハンドルを持つ
		};

		ColliderShape() : type(EShapeType::Sphere), sphere({ 1.0f }) {} // デフォルト
	};

	// コリジョンワールドに登録するインスタンス
	struct CollisionInstance
	{
		ECS::Entity entity = ECS::Limits::INVALID_ENTITY;	// エンティティID
		DirectX::XMFLOAT4X4 worldMat = {};					// ワールド行列

		//Handle<Resource::Model> modelHandle = {};			// モデルのハンドル
		ColliderShape collShape = {};						// 形状登録

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