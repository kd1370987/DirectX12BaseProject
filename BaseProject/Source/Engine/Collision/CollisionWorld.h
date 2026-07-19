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
		Handle<CollisionInstance> AllcateDynamicEntity(const CollisionInstance& a_instance);

		// フレームに一度だけ呼び出す
		// 生成時にメモリ確保用にインスタンス数を指定できる。超えてもいい。
		void BuildWorld(UINT a_resizeNum = 0);

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


	private:

		// 静的データ管理
		Storage::HandlePool<CollisionInstance> m_staticHandlePool = {};		// 静的ハンドルの管理
		std::vector<CollisionInstance> m_staticInstanceVec;					// 静的判定オブジェクト配列
		std::vector<Resource::BVHNode> m_staticNodeVec = {};				// ノード配列
		std::vector<int> m_staticInstanceIndexVec = {};				// 全インスタンスインデックス
		bool m_isStaticDirty = false;								// 更新の必要の有無
		int m_staticRootNodeIndex = 0;

		// 動的データ管理
		Storage::HandlePool<CollisionInstance> m_dynamicHandlePool = {};	// 動的ハンドルの管理
		std::vector<CollisionInstance> m_dynamicInstanceVec;				// 動的判定オブジェクト配列
		std::vector<Resource::BVHNode> m_dynamicNodeVec = {};				// ノード配列
		std::vector<int> m_dynamicInstanceIndexVec = {};				// 全インスタンスインデックス

		// ワールド全体のボックス
		DirectX::BoundingBox m_worldAABB = {};
	};
};