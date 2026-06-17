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

		// レイ判定
		bool Raycast(const RayInfo& a_ray,Result& a_outResult,const ECS::Entity& a_myID = ECS::Limits::INVALID_ENTITY);

	private:

		// 静的データ管理
		Storage::HandleStorage<CollisionInstance> m_staticHandleStorage = {};		// 静的ハンドルの管理
		std::vector<CollisionInstance> m_staticInstanceVec;					// 静的判定オブジェクト配列
		std::vector<Resource::BVHNode> m_staticNodeVec = {};				// ノード配列
		std::vector<int> m_staticInstanceIndexVec = {};				// 全インスタンスインデックス
		bool m_isStaticDirty = false;								// 更新の必要の有無
		int m_staticRootNodeIndex = 0;

		// 動的データ管理
		Storage::HandleStorage<CollisionInstance> m_dynamicHandleStorage = {};	// 動的ハンドルの管理
		std::vector<CollisionInstance> m_dynamicInstanceVec;				// 動的判定オブジェクト配列
		std::vector<Resource::BVHNode> m_dynamicNodeVec = {};				// ノード配列
		std::vector<int> m_dynamicInstanceIndexVec = {};				// 全インスタンスインデックス

		// ワールド全体のボックス
		DirectX::BoundingBox m_worldAABB = {};
	};
};