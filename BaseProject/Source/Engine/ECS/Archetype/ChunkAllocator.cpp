#include "ChunkAllocator.h"
namespace Engine::ECS
{
	void ChunkAllocator::Init(size_t a_blockChunkNum)
	{
		// ブロックの確保
		Expand(a_blockChunkNum);
	}
	Chunk* ChunkAllocator::Allocate()
	{
		// 空きチャンクがなければ新たにブロックを追加
		if (!m_pFreeHead)
		{
			Expand(m_chunkBlocks.begin()->chunkCount);
		}

		// フリーリストの更新
		Chunk* _pChunk = m_pFreeHead;
		m_pFreeHead = _pChunk->pNextFree;

		_pChunk->pNextFree = nullptr;
		_pChunk->count = 0;
		_pChunk->pArchetype = nullptr;

		return _pChunk;
	}
	void ChunkAllocator::Free(Chunk* a_pChunk)
	{
		a_pChunk->pNextFree = m_pFreeHead;
		m_pFreeHead = a_pChunk;
	}
	void ChunkAllocator::Expand(size_t a_chunkNum)
	{
		// チャンクブロック追加
		auto& _chunkBlock = m_chunkBlocks.emplace_back();

		// チャンクブ配列確保
		_chunkBlock.chunks.resize(a_chunkNum);
		_chunkBlock.chunkCount = static_cast<uint32_t>(a_chunkNum);

		// メモリの事前確保
		_chunkBlock.pChunksMemories =
			static_cast<std::byte*>(
				::operator new[](
					CHUNK_MEMORY_SIZE* a_chunkNum,
					std::align_val_t(CHUNK_MAX_ALIGMENT)
					)
				);

		// 各チャンクにメモリを割り当てていく
		for (uint32_t _i = 0; _i < _chunkBlock.chunkCount; ++_i)
		{
			Chunk& _chunk = _chunkBlock.chunks[_i];
			_chunk.data = reinterpret_cast<uint8_t*>(_chunkBlock.pChunksMemories + CHUNK_MEMORY_SIZE * _i);
			_chunk.count = 0;
			_chunk.pArchetype = nullptr;


			// 次の空きチャンクを指定
			m_pFreeHead = &_chunk;
		}
	}
}
