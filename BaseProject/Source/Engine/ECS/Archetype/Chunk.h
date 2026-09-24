#pragma once
namespace Engine::ECS
{
	struct Archetype;

	//==========================================================================================
	// チャンク
	//
	// 同じアーキタイプのエンティティを詰めて持つ固定長のメモリ塊。
	// 中身は SoA で、先頭にエンティティ配列、その後ろにコンポーネントごとの配列を並べる。
	// コンポーネント配列の置き場所(オフセット)はアーキタイプが持つ。
	// 容量やレイアウトはアーキタイプ単位で共通なので、チャンク自身は持たない。
	//
	// 実体とメモリは ChunkAllocator が持ち、ArchetypeManager が借りてアーキタイプに並べる
	//==========================================================================================
	struct Chunk
	{
		// エンティティの取得 : 配列外なら無効値を返す
		Entity GetEntity(uint32_t a_index) const
		{
			if (a_index >= count) return Limits::INVALID_ENTITY;
			return entityData[a_index];
		}

		Archetype*		pArchetype = nullptr;	// 所属しているアーキタイプ(レイアウトはここから引く)
		ECS::Entity*	entityData = nullptr;	// エンティティ配列(data の先頭を指す)
		uint8_t*		data = nullptr;			// バイトデータ
		uint32_t		count = 0;				// 現在のエンティティ数

		Chunk* pNextFree = nullptr;			// 次の空きチャンク(アロケーターが使う)
	};
}
