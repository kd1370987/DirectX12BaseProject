#pragma once
namespace Engine::Collision
{
	// 形状
	enum class EShapeType : uint32_t
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
			struct { Math::Vector3 extents; } box;			// ボックス
			struct { float radius; float height; } capsule;		// カプセル
			Handle<Resource::Model> modelHandle;				// メッシュの場合はモデルのハンドルを持つ
		};

		ColliderShape() : type(EShapeType::Sphere), sphere({ 1.0f }) {} // デフォルト
	};

	// コリジョンワールドに登録するインスタンス
	struct CollisionInstance
	{
		ECS::Entity entity = ECS::Limits::INVALID_ENTITY;	// エンティティID
		Math::Matrix worldMat = {};					// ワールド行列

		ColliderShape collShape = {};						// 形状登録

		// モデル全体のAABBをワールド空間に変換したボックス
		DirectX::BoundingBox worldAABB = {};

		// このインスタンスが属するレイヤー(アプリ側の Layer をそのまま入れる)。
		// クエリ側はこれをマスクで見て、当たりに行かない相手を先に弾く。
		// 0 のままなら「レイヤー未設定」として弾かない(既存の登録元を壊さないため)
		uint32_t layer = 0;
	};

	// レイヤーマスクの既定値。すべてのレイヤーが対象(=今までどおり弾かない)
	inline constexpr uint32_t kLayerMaskAll = 0xFFFFFFFFu;

	// ヒットした際に帰ってくる情報
	struct Result
	{
		ECS::Entity hitEntity = ECS::Limits::INVALID_ENTITY;
		Math::Vector3 hitPos = { 0.0f,0.0f,0.0f };
		Math::Vector3 hitNormal = { 0.0f,0.0f,0.0f };
		float hitDistance = 0.0f;
		bool isHit = false;
	};

	// 押し出し（デペネトレーション）用の接触情報
	struct Contact
	{
		Math::Vector3 normal = {};	// 押し出す向き（三角形→プリミティブ）
		float depth = 0.0f;			// めり込み量
		bool hit = false;			// 接触しているか
	};

	// レイ判定時に渡す情報。実体はエディターとも共有する Math::Ray
	using RayInfo = Math::Ray;

	// 球判定時に渡す情報
	struct SphereInfo
	{
		Math::Vector3 origin = {};		// 中心点
		float radius = {};
	};

	// カプセル判定時に渡す情報（線分＋半径）
	struct CapsuleInfo
	{
		Math::Vector3 pointA = {};		// 線分の端点A
		Math::Vector3 pointB = {};		// 線分の端点B
		float radius = 0.0f;			// 半径
	};
}