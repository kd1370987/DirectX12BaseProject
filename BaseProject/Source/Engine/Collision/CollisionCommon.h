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

	// レイ判定時に渡す情報
	struct RayInfo
	{
		Math::Vector3 origin = { 0.0f,0.0f,0.0f };
		Math::Vector3 direction = { 0.0f,0.0f,1.0f };
		float maxDistance = 1000.0f;
	};

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

	// ボックス(AABB)判定時に渡す情報（ワールド軸に平行）
	struct BoxInfo
	{
		Math::Vector3 center = {};		// 中心点
		Math::Vector3 extents = {};		// 各軸の半分の長さ
	};

	// OBB判定時に渡す情報（向きあり）
	struct OBBInfo
	{
		Math::Vector3 center = {};						// 中心点
		Math::Vector3 extents = {};						// 各軸の半分の長さ
		Math::Quaternion orientation = Math::Quaternion::Identity();	// 回転
	};

	// フラスタム判定時に渡す情報（DirectX::BoundingFrustum と同じパラメータ）
	struct FrustumInfo
	{
		Math::Vector3 origin = {};						// 視点
		Math::Quaternion orientation = Math::Quaternion::Identity();	// 向き

		// 各面の傾き（近平面から見たスロープ）
		float rightSlope = 1.0f;
		float leftSlope = -1.0f;
		float topSlope = 1.0f;
		float bottomSlope = -1.0f;

		float nearPlane = 0.1f;		// 近平面までの距離
		float farPlane = 100.0f;	// 遠平面までの距離
	};
}