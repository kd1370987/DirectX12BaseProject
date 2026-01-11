#include "ArchetypeChunkManager.h"

#include "../ArchetypeChunk/ArchetypeChunk.h"
#include "../ComponentMetaRegistry/ComponentMetaRegistry.h"

ArchetypeChunkManager::ArchetypeChunkManager()
{
}

ArchetypeChunkManager::~ArchetypeChunkManager()
{
	// メモリの解放
	for (auto& [_sig, _chunkVec] : m_pArchetypeChunkMap)
	{
		for (auto* _chunk : _chunkVec)
		{	
			delete[] _chunk->entityData;

			operator delete[](
				_chunk->data,
				std::align_val_t(_chunk->maxAlign)
			);

			delete _chunk;
		}
	}
}

void ArchetypeChunkManager::Init(ComponentMetaRegistry* a_pMetaRegister)
{
	m_pMetaRegister = a_pMetaRegister;
}

const std::vector<ArchetypeChunk*>& ArchetypeChunkManager::GetArchetypeChunk(const ECS::Signature& a_sig)
{
	auto _it = m_pArchetypeChunkMap.find(a_sig);
	if (_it != m_pArchetypeChunkMap.end())
	{
		return _it->second;
	}

	assert(0 && "登録されていないアーキタイプです");
	return std::vector<ArchetypeChunk*>();
}

std::vector<ArchetypeChunk*> ArchetypeChunkManager::MatchingArchetypeChunkVec(const ECS::Signature& a_sig)
{
	std::vector<ArchetypeChunk*> _matches;
	_matches.reserve(24);

	for (auto& [_sig, _chunkVec] : m_pArchetypeChunkMap)
	{
		// and検索
		if ((_sig & a_sig) == a_sig)
		{
			_matches.insert(_matches.end(), _chunkVec.begin(),_chunkVec.end());
		}
	}

	return _matches;
}

EntityLocation ArchetypeChunkManager::AllocateEntity(const ECS::Entity& a_entity, const ECS::Signature& a_sig)
{
	EntityLocation _loca = {};

	// すでに登録されているアーキタイプか確認
	auto _it = m_pArchetypeChunkMap.find(a_sig);
	if (_it != m_pArchetypeChunkMap.end())
	{
		// 空いてるチャンクを探す
		for (auto* _chunk : _it->second)
		{
			// 空いているチャンクがあれば割り当てる
			if (_chunk->capacity > _chunk->count)
			{
				_chunk->entityData[_chunk->count] = a_entity;

				_loca.pArchetypeChunk = _chunk;
				_loca.chunkIndex = _chunk->count;

				_chunk->count++;

				return _loca;
			}
		}
	}

	// 生成して登録
	ArchetypeChunk* _newChunk = CreateArchetypeChunk(a_sig);
	m_pArchetypeChunkMap[a_sig].push_back(_newChunk);
	_newChunk->entityData[_newChunk->count] = a_entity;

	_loca.pArchetypeChunk = _newChunk;
	_loca.chunkIndex = _newChunk->count;

	_newChunk->count++;

	return _loca;
}

uint8_t* ArchetypeChunkManager::RefComponent(const EntityLocation& a_loca, const ECS::ComponentTypeID& a_typeID)
{
	ArchetypeChunk* _chunk = a_loca.pArchetypeChunk;
	size_t _offset = 0;
	size_t _stride = 0;

	auto _it = _chunk->layoutMap.find(a_typeID);
	if (_it != _chunk->layoutMap.end())
	{
		_offset = _it->second.offset;
		_stride = _it->second.stride;
	}

	return _chunk->data + _offset + (_stride * a_loca.chunkIndex);
}

uint8_t* ArchetypeChunkManager::RefComponentArray(ArchetypeChunk* a_chunk, const ECS::ComponentTypeID& a_typeID)
{
	ArchetypeChunk* _chunk = a_chunk;
	size_t _offset = 0;
	size_t _stride = 0;

	auto _it = _chunk->layoutMap.find(a_typeID);
	if (_it != _chunk->layoutMap.end())
	{
		_offset = _it->second.offset;
		_stride = _it->second.stride;
	}

	return _chunk->data + _offset;
}

ArchetypeChunk* ArchetypeChunkManager::CreateArchetypeChunk(const ECS::Signature& a_sig)
{
	
	// チャンクの生成
	ArchetypeChunk* _chunk = new ArchetypeChunk;
	size_t _chunkMemorySize = 64 * 1024;			// 64kbのサイズを確保
	
	// メモリレイアウト計算
	CalcChunkLayout(_chunk,a_sig,_chunkMemorySize);

	// 初期化
	_chunk->count = 0;

	// メモリ確保
	_chunk->entityData = new ECS::Entity[_chunk->capacity];
	_chunk->data = reinterpret_cast<uint8_t*>(
		operator new[](_chunkMemorySize,std::align_val_t(_chunk->maxAlign))
	);

	return _chunk;
}

void ArchetypeChunkManager::CalcChunkLayout(ArchetypeChunk* a_pChunk, const ECS::Signature& a_sig, size_t a_memorySize)
{
	// 1エンティティが消費するバイト数を計算
	size_t _entityStride = 0;		// 1エンティティが消費するバイト数
	size_t _maxAligne = 1;			// コンポーネントのアライメント
	for (ECS::ComponentTypeID _comTypeID = 0; _comTypeID < a_sig.size(); ++_comTypeID)
	{
		// コンポーネント登録チェック
		if (!a_sig.test(_comTypeID)) continue;

		// メタ情報の取得
		const ComponentMeta& _data = m_pMetaRegister->GetMetaData(_comTypeID);
		_entityStride += _data.compAlignSize;
		_maxAligne = std::max(_maxAligne, _data.compAlign);
	}

	// 1エンティティが消費するメモリサイズ
	_entityStride = Alignment::Up(_entityStride, _maxAligne);

	// １チャンクのキャパシティ決定
	a_pChunk->capacity = static_cast<uint32_t>(a_memorySize / _entityStride);


	// オフセット位置計算
	size_t _offset = 0;
	for (ECS::ComponentTypeID _comTypeID = 0; _comTypeID < a_sig.size(); ++_comTypeID)
	{
		// コンポーネント登録チェック
		if (!a_sig.test(_comTypeID)) continue;

		const ComponentMeta& _meta = m_pMetaRegister->GetMetaData(_comTypeID);
		_offset = Alignment::Up(_offset,_meta.compAlign);

		Layout _lay = {};
		_lay.offset = _offset;
		_lay.stride = m_pMetaRegister->GetMetaData(_comTypeID).compAlignSize;
		a_pChunk->layoutMap.emplace(_comTypeID, _lay);
		_offset += _meta.compSize * a_pChunk->capacity;
	}

	a_pChunk->maxAlign = _maxAligne;
}
