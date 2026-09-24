#pragma once

#include "Chunk.h"

namespace Engine::ECS
{
	//==========================================================================================
	// チャンクアロケーター
	//
	// チャンクを局所的に一括で管理するクラス
	// チャンクは配列で一括確保して足りなくなればブロックを足していく方式
	//
	// チャンクのデータ領域の先頭にはエンティティ配列を置く(entityData == data)。
	// 貸し出したチャンクのアドレスはブロックを足しても動かない
	//==========================================================================================
	class ChunkAllocator
	{
	public:

		// 1チャンクのデータ領域のサイズ
		static constexpr size_t CHUNK_MEMORY_SIZE = 64 * 1024;

		// データ領域の先頭アライメント : コンポーネントのアライメントはこれ以下であること
		static constexpr size_t CHUNK_MAX_ALIGNMENT = 256;

		ChunkAllocator();
		~ChunkAllocator();

		// コピー禁止(チャンクのメモリを所有しているため)
		NON_COPYABLE_NON_MOVABLE(ChunkAllocator);

		/// <summary>
		/// チャンクブロックの設定と最初のブロックの確保
		/// </summary>
		/// <param name="a_blockChunkNum">1ブロックあたりのチャンク数</param>
		void Init(size_t a_blockChunkNum);

		/// <summary>
		/// チャンクの貸し出し : データ領域は0クリアして返す
		/// </summary>
		/// <param name="a_pArchetype">貸し出し先のアーキタイプ</param>
		Chunk* Allocate(Archetype* a_pArchetype);

		// チャンクの返却
		void Free(Chunk* a_pChunk);

	private:

		/// <summary>
		/// チャンクブロックの追加確保
		/// </summary>
		/// <param name="a_chunkNum">チャンク数</param>
		void Expand(size_t a_chunkNum);

	private:

		// 1ブロック分の確保データ : メモリの持ち主
		struct ChunkBlock
		{
			explicit ChunkBlock(size_t a_chunkNum);
			~ChunkBlock();

			// 貸し出した Chunk* が動かないよう、コピーもムーブもさせない
			NON_COPYABLE_NON_MOVABLE(ChunkBlock);

			std::vector<Chunk>	chunks;
			std::byte*			pChunksMemories = nullptr;
		};

		// チャンクブロック : 一括で確保してアーキタイプに貸し出す
		std::vector<std::unique_ptr<ChunkBlock>> m_upChunkBlocks;

		// 1ブロックあたりのチャンク数
		size_t m_blockChunkNum = 0;

		// 空きチャンクの先頭
		Chunk* m_pFreeHead = nullptr;
	};
}
