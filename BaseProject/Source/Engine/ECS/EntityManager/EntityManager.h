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

	const EntityLocation& GetLocation(const ECS::Entity& a_entity);


private:

	
	std::vector<EntityLocation>		m_entityLocationVec;	// エンティティの住所
	std::vector<ECS::Signature>		m_signatureVec;			// エンティティとシグネチャを紐づけるもの
	std::vector<ECS::Generation>	m_entityGeneVec;		// 世代を含めたエンティティリスト
	
	// 次に使用するEntityのインデックス
	std::queue<ECS::EntityIndex> m_availbleEntitiyQueue;

	UINT m_aliveCount = 0;
};