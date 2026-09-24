#pragma once

#include "Chunk.h"

namespace Engine::ECS
{
	//==========================================================================================
	// チャンクアロケーター
	//
	// チャンクを局所的に一括で管理するクラス
	// チャンクは配列で一括確保して足りなくなればブロックを足していく方式
	//==========================================================================================
	class ChunkAllocator
	{
	public:

		// チャンクブロックの設定
		void Init(size_t a_blockChunkNum);

		// チャンクの貸し出し
		Chunk* Allocate();

		// チャンクの返却
		void Free(Chunk* a_pChunk);

	private:

		/// <summary>
		/// チャンクブロックの追加確保
		/// </summary>
		/// <param name="a_chunkNum">チャンク数</param>
		void Expand(size_t a_chunkNum);

	private:


		// 1チャンクのデータ領域のサイズ
		static constexpr size_t CHUNK_MEMORY_SIZE = 64 * 1024;
		static constexpr size_t CHUNK_MAX_ALIGMENT = 256;

		// 1チャンク分の確保データ
		struct ChunkBlock
		{
			std::vector<Chunk>	chunks;
			std::byte*			pChunksMemories;

			uint32_t chunkCount = 0;
		};

		// チャンクブロック : 一括で確保してアーキタイプに貸し出す
		std::vector<ChunkBlock> m_chunkBlocks;

		// 空きチャンクの先頭
		Chunk* m_pFreeHead = nullptr;
	};
}