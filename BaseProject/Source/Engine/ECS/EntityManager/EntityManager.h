#pragma once

#include "../Internal/EntityLocation.h"

namespace Engine::ECS
{


	class EntityManager
	{
	public:

		EntityManager();
		~EntityManager();

		/// <summary>
		/// 初期化
		/// </summary>
		void Init();


		/// <summary>
		/// エンティティの作成
		/// </summary>
		/// <returns>作成されたEntityが返される</returns>
		Entity CreateEntity(const Signature& a_sig);

		/// <summary>
		/// エンティティの削除
		/// </summary>
		void DestroyEntity(const Entity& a_entity);

		/// <summary>
		/// エンティティのロケーションを記憶
		/// </summary>
		/// <param name="a_loca">ロケーションデータ</param>
		void SetEntityLocation(const Entity& a_entity, const EntityLocation& a_loca);

		/// <summary>
		/// エンティティを指定してロケーションを取得
		/// </summary>
		const EntityLocation& GetLocation(const Entity& a_entity);

		// ロケーション操作
		EntityLocation& RefEntityLocation(const Entity& a_entity);

		// アクセサ
		const std::vector<EntityLocation>& GetAllEntityLocation();	// エンティティの場所配列を返す
		UINT GetAliveEntityCount();									// 生存しているエンティティの数

		// エンティティとシグネチャ
		const Signature& GetSignature(const Entity& a_entity);					// シグネチャの取得
		void SetSignature(const Entity& a_entity, const Signature& a_sig);		// シグネチャのセット

	private:

		// ヘルパー関数
		uint32_t GetGeneration(const Entity& a_entity);		// 世代取得
		uint32_t GetIndex(const Entity& a_entity);			// インデックス取得

	private:

		std::vector<EntityLocation>	m_entityLocationVec;	// エンティティの住所
		std::vector<Signature>		m_signatureVec;			// エンティティとシグネチャを紐づけるもの
		std::vector<Generation>		m_entityGeneVec;		// 世代を含めたエンティティリスト

		// 次に使用するEntityのインデックス
		std::queue<EntityIndex> m_availbleEntitiyQueue;

		// 生存中のエンティティ
		UINT m_aliveCount = 0;
	};

}