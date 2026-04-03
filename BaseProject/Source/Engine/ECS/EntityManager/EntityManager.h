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

		/// <summary>
		/// 全エンティティのロケーションを返す
		/// </summary>
		const std::vector<EntityLocation>& GetAllEntityLocation();

		/// <summary>
		/// 現在のエンティティの生存数を返す
		/// </summary>
		UINT GetAliveEntityCount();

		/// <summary>
		/// エンティティのシグネチャを取得する
		/// </summary>
		const Signature& GetSignature(const Entity& a_entity);

	private:

		uint32_t GetGeneration(const Entity& a_entity);
		uint32_t GetIndex(const Entity& a_entity);

	private:

		std::vector<EntityLocation>		m_entityLocationVec;	// エンティティの住所
		std::vector<Signature>		m_signatureVec;			// エンティティとシグネチャを紐づけるもの
		std::vector<Generation>	m_entityGeneVec;		// 世代を含めたエンティティリスト

		// 次に使用するEntityのインデックス
		std::queue<EntityIndex> m_availbleEntitiyQueue;

		// 生存中のエンティティ
		UINT m_aliveCount = 0;
	};

}