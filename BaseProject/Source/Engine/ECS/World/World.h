#pragma once

#include "../EntityManager/EntityManager.h"
#include "../SystemManager/SystemManager.h"
#include "../ArchetypeChunkManager/ArchetypeChunkManager.h"
#include "../ArchetypeChunk/ArchetypeChunk.h"

#include "../ComponentMetaRegistry/ComponentMetaRegistry.h"

#include "../Internal/SystemComon.h"

class World
{
public:

	/// <summary>
	/// 初期化
	/// </summary>
	void Init();

	/// <summary>
	/// 解放
	/// </summary>
	void Release();

	/// <summary>
	/// すべてのデータのリセット
	/// </summary>
	void ClaerMemory();

	/// <summary>
	/// エンティティの生成
	/// </summary>
	/// <param name="a_sig">アーキタイプを指定</param>
	ECS::Entity CreateEntity(const ECS::Signature& a_sig);

	/// <summary>
	/// 型情報からIDを取得
	/// </summary>
	/// <param name="a_index"></param>
	/// <returns></returns>
	ECS::ComponentTypeID GetCompTypeID(const std::type_index& a_index);

	/// <summary>
	/// データの取得
	/// </summary>
	/// <param name="a_entity"></param>
	/// <param name="a_index"></param>
	/// <returns></returns>
	uint8_t* RefData(const ECS::Entity& a_entity, const std::type_index& a_index);

	void RunSystem(SystemType a_type,float a_dt);

	template<typename Comp>
	Comp* GetComponentArray(ArchetypeChunk* a_chunk)
	{
		return reinterpret_cast<Comp*>(m_upArchetypeChunkManager->RefComponentArray(a_chunk, m_spComponentMetaRegistry->GetTypeID<Comp>()));
	}

	template<typename... Components,typename Func>
	void ForEach(Func a_func)
	{
		// シグネチャを生成
		ECS::Signature _sig;
		(_sig.set(m_spComponentMetaRegistry->GetTypeID<Components>()), ...);

		// チャンクの配列を取得
		for (auto* _chunk : m_upArchetypeChunkManager->MatchingArchetypeChunkVec(_sig))
		{
			if (!_chunk || _chunk->count == 0) continue;

			// 操作しやすいように配列にして返す
			auto _arrays = std::forward_as_tuple(
				GetComponentArray<Components>(_chunk)...
			);

			std::apply(
				[&](auto... a_data)
				{
					a_func(_chunk,_chunk->count,a_data...);
				},
				_arrays
			);
		}

	};

	

private:

	// マネージャー軍
	std::unique_ptr<EntityManager> m_upEntityManager = nullptr;
	std::unique_ptr<SystemManager> m_upSystemManager = nullptr;
	std::unique_ptr<ArchetypeChunkManager> m_upArchetypeChunkManager = nullptr;

	std::shared_ptr<ComponentMetaRegistry> m_spComponentMetaRegistry = nullptr;


// シングルトン
private:

	World();
	~World();

	// コピー禁止
	World(const World&) = default;
	World& operator=(const World&) = default;

	// ムーブ禁止
	World(World&&) = default;
	World& operator = (World&&) = default;

public:

	static World& Instance()
	{
		static World _instance;
		return _instance;
	};
};