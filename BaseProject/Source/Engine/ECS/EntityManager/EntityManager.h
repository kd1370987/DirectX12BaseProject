#pragma once

#include "../Internal/EntityLocation.h"

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
	ECS::Entity CreateEntity(const ECS::Signature& a_sig);
	
	/// <summary>
	/// エンティティの削除
	/// </summary>
	void DestroyEntity(const ECS::Entity& a_entity);

	/// <summary>
	/// エンティティのロケーションを記憶
	/// </summary>
	/// <param name="a_loca">ロケーションデータ</param>
	void SetEntityLocation(const ECS::Entity& a_entity,const EntityLocation& a_loca);

	/// <summary>
	/// エンティティを指定してロケーションを取得
	/// </summary>
	const EntityLocation& GetLocation(const ECS::Entity& a_entity);

	// ロケーション操作
	EntityLocation& RefEntityLocation(const ECS::Entity& a_entity);

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
	const ECS::Signature& GetSignature(const ECS::Entity& a_entity);

private:

	uint32_t GetGeneration(const ECS::Entity& a_entity);
	uint32_t GetIndex(const ECS::Entity& a_entity);

private:

	std::vector<EntityLocation>		m_entityLocationVec;	// エンティティの住所
	std::vector<ECS::Signature>		m_signatureVec;			// エンティティとシグネチャを紐づけるもの
	std::vector<ECS::Generation>	m_entityGeneVec;		// 世代を含めたエンティティリスト
	
	// 次に使用するEntityのインデックス
	std::queue<ECS::EntityIndex> m_availbleEntitiyQueue;

	// 生存中のエンティティ
	UINT m_aliveCount = 0;
};