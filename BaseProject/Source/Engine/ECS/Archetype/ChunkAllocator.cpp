#include "ChunkAllocator.h"
namespace Engine::ECS
{
	//======================================================================================
	// チャンクブロック
	//======================================================================================
	ChunkAllocator::ChunkBlock::ChunkBlock(size_t a_chunkNum)
	{
		chunks.resize(a_chunkNum);

		// メモリの事前確保
		pChunksMemories =
			static_cast<std::byte*>(
				::operator new[](
					CHUNK_MEMORY_SIZE * a_chunkNum,
					std::align_val_t(CHUNK_MAX_ALIGNMENT)
					)
				);
	}

	ChunkAllocator::ChunkBlock::~ChunkBlock()
	{
		::operator delete[](pChunksMemories, std::align_val_t(CHUNK_MAX_ALIGNMENT));
	}

	//======================================================================================
	// チャンクアロケーター
	//======================================================================================
	ChunkAllocator::ChunkAllocator()
	{}

	ChunkAllocator::~ChunkAllocator()
	{
		// メモリはブロックが持っているので、ブロックを捨てれば返る
		m_pFreeHead = nullptr;
		m_upChunkBlocks.clear();
		m_totalChunkCount = 0;
		m_freeChunkCount = 0;
	}

	void ChunkAllocator::Init(size_t a_blockChunkNum)
	{
		ENGINE_ERRLOG(a_blockChunkNum > 0, "ChunkAllocator : 初期化時のチャンク数が０です");
		if (a_blockChunkNum == 0) a_blockChunkNum = 1;

		// 2回目以降の初期化ではブロックを足さない(確保済みのものを使い回す)
		m_blockChunkNum = a_blockChunkNum;
		if (!m_upChunkBlocks.empty()) return;

		// ブロックの確保
		Expand(m_blockChunkNum);
	}

	Chunk* ChunkAllocator::Allocate(Archetype* a_pArchetype)
	{
		ENGINE_ERRLOG(m_blockChunkNum > 0, "ChunkAllocator : 初期化前に貸し出そうとしました");

		// 空きチャンクがなければ新たにブロックを追加
		if (!m_pFreeHead)
		{
			Expand(m_blockChunkNum > 0 ? m_blockChunkNum : 1);
		}

		// フリーリストの更新
		Chunk* _pChunk = m_pFreeHead;
		m_pFreeHead = _pChunk->pNextFree;
		--m_freeChunkCount;

		_pChunk->pNextFree = nullptr;
		_pChunk->count = 0;
		_pChunk->pArchetype = a_pArchetype;

		// 使い回しのチャンクには前の持ち主のデータが残っているので消す
		std::memset(_pChunk->data, 0, CHUNK_MEMORY_SIZE);

		return _pChunk;
	}

	void ChunkAllocator::Free(Chunk* a_pChunk)
	{
		if (!a_pChunk) return;

		// 貸し出し中のチャンクは必ずアーキタイプを持っている。持っていなければ二重返却
		ENGINE_ERRLOG(a_pChunk->pArchetype != nullptr, "ChunkAllocator : チャンクが二重に返却されました");
		if (!a_pChunk->pArchetype) return;

		a_pChunk->pArchetype = nullptr;
		a_pChunk->count = 0;

		a_pChunk->pNextFree = m_pFreeHead;
		m_pFreeHead = a_pChunk;
		++m_freeChunkCount;
	}

	void ChunkAllocator::Expand(size_t a_chunkNum)
	{
		// チャンクブロック追加
		auto& _upChunkBlock = m_upChunkBlocks.emplace_back(std::make_unique<ChunkBlock>(a_chunkNum));

		// 各チャンクにメモリを割り当てていく。
		// 後ろから積むことで、アドレスの若い順に貸し出される
		for (size_t _i = a_chunkNum; _i-- > 0;)
		{
			Chunk& _chunk = _upChunkBlock->chunks[_i];
			_chunk.data = reinterpret_cast<uint8_t*>(_upChunkBlock->pChunksMemories + CHUNK_MEMORY_SIZE * _i);
			_chunk.entityData = reinterpret_cast<Entity*>(_chunk.data);	// 先頭はエンティティ配列
			_chunk.count = 0;
			_chunk.pArchetype = nullptr;

			// 次の空きチャンクを指定
			_chunk.pNextFree = m_pFreeHead;
			m_pFreeHead = &_chunk;
		}

		m_totalChunkCount += a_chunkNum;
		m_freeChunkCount += a_chunkNum;
	}
}
