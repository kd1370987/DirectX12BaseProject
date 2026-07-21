#pragma once
namespace Engine::Collision
{
	// 当たり判定を処理する専用の空間
	class CollisionWorld
	{
	public:

		CollisionWorld() = default;
		~CollisionWorld() = default;

		// インスタンスの登録
		// 削除、移動、エディター時の変更用にハンドルを確保しておく
		Handle<CollisionInstance> AllcateStaticEntity(const CollisionInstance& a_instance);
		void AllcateDynamicEntity(const CollisionInstance& a_instance);

		// フレームに一度だけ呼び出す（静的TLASの構築＋デバッグ描画）
		void BuildWorld();

		// 動的TLASを構築する。
		// 動的コライダーの submit 後、判定クエリ（Physicsフェーズ）の前に呼ぶこと。
		void BuildDynamicWorld();

		// フレームの初めに呼び出す。
		// 動的ワールドを空にする（インスタンス／ノード／インデックスすべて）
		void ClearDynamicWorld(size_t a_size);

		// シーンの更新時などに呼び出す。ワールドのリセット
		void Clear();

		// デバッグ用でワールドを見るための関数
		void DrawDebug();

		// レイ判定
		bool Raycast(const RayInfo& a_ray,Result& a_outResult,const ECS::Entity& a_myID = ECS::Limits::INVALID_ENTITY);

		// ---- オーバーラップ系クエリ（触れているエンティティを1つ返す） ----
		// いずれもワールド空間のプリミティブを渡す。a_myID は自分自身を除外するためのID。

		// 球判定
		bool VsSphere(const SphereInfo& a_info,Result& a_outResult,const ECS::Entity& a_myID = ECS::Limits::INVALID_ENTITY);

		// カプセル判定
		bool VsCapsule(const CapsuleInfo& a_info,Result& a_outResult,const ECS::Entity& a_myID = ECS::Limits::INVALID_ENTITY);

		// ボックス(AABB)判定
		bool VsBox(const BoxInfo& a_info,Result& a_outResult,const ECS::Entity& a_myID = ECS::Limits::INVALID_ENTITY);

		// OBB判定
		bool VsOBB(const OBBInfo& a_info,Result& a_outResult,const ECS::Entity& a_myID = ECS::Limits::INVALID_ENTITY);

		// フラスタム判定
		bool VsFrustum(const FrustumInfo& a_info,Result& a_outResult,const ECS::Entity& a_myID = ECS::Limits::INVALID_ENTITY);

		// ---- 押し出し（デペネトレーション） ----
		// 静的メッシュから押し出す。反復して複数面（床＋壁など）を解決する。

		// カプセル（球は pointA==pointB で表現）。
		// a_pointA / a_pointB は押し出し後の位置に更新され、a_outCorrection に合計補正が入る。
		bool ResolveCapsule(DXSM::Vector3& a_pointA, DXSM::Vector3& a_pointB, float a_radius,
			const ECS::Entity& a_myID, DXSM::Vector3& a_outCorrection, int a_iterations = 4);

		// 球（内部でカプセル押し出しを流用）。a_center は押し出し後に更新される。
		bool ResolveSphere(DXSM::Vector3& a_center, float a_radius,
			const ECS::Entity& a_myID, DXSM::Vector3& a_outCorrection, int a_iterations = 4);
	private:

		// TLASの再構築
		void ReBuildStaticTLAS();
		void ReBuildDynamicTLAS();

	private:

		// 静的データ管理
		Storage::HandlePool<CollisionInstance> m_staticHandlePool = {};		// 静的ハンドルの管理
		std::vector<CollisionInstance> m_staticInstanceVec;					// 静的判定オブジェクト配列
		std::vector<Resource::BVHNode> m_staticNodeVec = {};				// ノード配列
		std::vector<int> m_staticInstanceIndexVec = {};				// 全インスタンスインデックス
		bool m_isStaticDirty = false;								// 更新の必要の有無
		int m_staticRootNodeIndex = 0;

		// 動的データ管理 : ダイナミックTLASは毎フレーム詰めなおし
		std::vector<CollisionInstance> m_dynamicInstanceVec;				// 動的判定オブジェクト配列
		std::vector<Resource::BVHNode> m_dynamicNodeVec = {};				// ノード配列
		std::vector<int> m_dynamicInstanceIndexVec = {};				// 全インスタンスインデックス
		int m_dynamicRootNodeIndex = 0;

		// ワールド全体のボックス
		DirectX::BoundingBox m_worldAABB = {};
	};
};