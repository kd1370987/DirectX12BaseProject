#include "ArchetypeManager.h"

#include "../../Core/Chunk.h"
#include "../../Internal/Archetype.h"
#include "../../ComponentMetaRegistry/ComponentMetaRegistry.h"

namespace Engine::ECS
{
	ArchetypeManager::ArchetypeManager()
	{}

	ArchetypeManager::~ArchetypeManager()
	{
		ReleaseAllChunks();
	}

	void ArchetypeManager::Init(ComponentMetaRegistry* a_pMetaRegister)
	{
		m_pMetaRegister = a_pMetaRegister;
		m_generation = 0;
	}

	const Archetype* ArchetypeManager::GetArchetype(const Signature& a_sig) const
	{
		auto _it = m_pArchetypeMap.find(a_sig);
		if (_it == m_pArchetypeMap.end()) return nullptr;

		return _it->second;
	}

	std::vector<Archetype*> ArchetypeManager::MatchingArchetypeVec(const Signature& a_sig, const Signature& a_excludeSig)
	{
		std::vector<Archetype*> _matches;
		_matches.reserve(24);

		for (auto& _upArchetype : m_upArchetypeVec)
		{
			const Signature& _sig = _upArchetype->signature;

			// AND検索 かつ 除外を1つも持っていない
			if (((_sig & a_sig) == a_sig) && ((_sig & a_excludeSig).none()))
			{
				_matches.push_back(_upArchetype.get());
			}
		}

		return _matches;
	}

	std::vector<Chunk*> ArchetypeManager::MatchingChunkVec(const Signature& a_sig, const Signature& a_excludeSig)
	{
		std::vector<Chunk*> _matches;
		_matches.reserve(24);

		for (Archetype* _pArchetype : MatchingArchetypeVec(a_sig, a_excludeSig))
		{
			_matches.insert(_matches.end(), _pArchetype->chunks.begin(), _pArchetype->chunks.end());
		}

		return _matches;
	}

	EntityLocation ArchetypeManager::AllocationEntity(const Entity& a_entity, const Signature& a_sig)
	{
		Archetype* _pArchetype = GetOrCreateArchetype(a_sig);

		// 空いているチャンクを探す。無ければ足す
		Chunk* _pChunk = nullptr;
		for (Chunk* _pCandidate : _pArchetype->chunks)
		{
			if (_pCandidate->count < _pArchetype->chunkCapacity)
			{
				_pChunk = _pCandidate;
				break;
			}
		}
		if (!_pChunk)
		{
			_pChunk = CreateChunk(_pArchetype);
		}

		// 末尾に詰める
		EntityLocation _loca = {};
		_loca.pChunk = _pChunk;
		_loca.chunkIndex = _pChunk->count;

		_pChunk->entityData[_pChunk->count] = a_entity;
		_pChunk->count++;

		return _loca;
	}

	uint8_t* ArchetypeManager::RefComponent(const EntityLocation& a_loca, const ComponentTypeID& a_typeID)
	{
		Chunk* _pChunk = a_loca.pChunk;
		if (!_pChunk || !_pChunk->pArchetype) return nullptr;

		// このアーキタイプが持っていないコンポーネントは nullptr
		const auto& _layoutMap = _pChunk->pArchetype->layoutMap;
		auto _it = _layoutMap.find(a_typeID);
		if (_it == _layoutMap.end()) return nullptr;

		const Layout& _layout = _it->second;
		return _pChunk->data + _layout.offset + (_layout.stride * a_loca.chunkIndex);
	}

	uint8_t* ArchetypeManager::RefComponentArray(Chunk* a_pChunk, const ComponentTypeID& a_typeID)
	{
		if (!a_pChunk || !a_pChunk->pArchetype) return nullptr;

		// 持っていないコンポーネントは nullptr(RefComponent と同じ理由)
		const auto& _layoutMap = a_pChunk->pArchetype->layoutMap;
		auto _it = _layoutMap.find(a_typeID);
		if (_it == _layoutMap.end()) return nullptr;

		return a_pChunk->data + _it->second.offset;
	}

	std::pair<Entity, uint32_t> ArchetypeManager::RemoveEntity(const EntityLocation& a_location)
	{
		Chunk* _pChunk = a_location.pChunk;
		const uint32_t _idx = a_location.chunkIndex;
		const uint32_t _lastIdx = _pChunk->count - 1;

		// スワップしたエンティティとインデックス。
		// 末尾を消したときは誰も動かないので INVALID のまま返す
		std::pair<Entity, uint32_t> _swapEntity = { Limits::INVALID_ENTITY, 0 };

		// 削除するエンティティが最後のエンティティでは無ければ、末尾を穴へ詰める
		if (_idx != _lastIdx)
		{
			// すべてのコンポーネント配列に対して同じ操作をする
			for (auto& [_compID, _layout] : _pChunk->pArchetype->layoutMap)
			{
				void* _removeData = _pChunk->data + _layout.offset + (_layout.stride * _idx);	// 削除データ
				void* _lastData = _pChunk->data + _layout.offset + (_layout.stride * _lastIdx);	// 最後データ

				std::memcpy(_removeData, _lastData, _layout.stride);
			}

			// エンティティ配列も移動
			const Entity _moved = _pChunk->entityData[_lastIdx];
			_pChunk->entityData[_idx] = _moved;

			_swapEntity = { _moved, _idx };
		}

		// チャンクのサイズをデクリメント
		--_pChunk->count;

		return _swapEntity;
	}

	Archetype* ArchetypeManager::GetOrCreateArchetype(const Signature& a_sig)
	{
		auto _it = m_pArchetypeMap.find(a_sig);
		if (_it != m_pArchetypeMap.end()) return _it->second;

		return CreateArchetype(a_sig);
	}

	Archetype* ArchetypeManager::CreateArchetype(const Signature& a_sig)
	{
		auto _upArchetype = std::make_unique<Archetype>();
		_upArchetype->signature = a_sig;

		// レイアウトと容量はアーキタイプ単位で1度だけ計算する
		CalcChunkLayout(_upArchetype.get(), CHUNK_MEMORY_SIZE);

		Archetype* _pArchetype = _upArchetype.get();
		m_upArchetypeVec.push_back(std::move(_upArchetype));
		m_pArchetypeMap.emplace(a_sig, _pArchetype);

		return _pArchetype;
	}

	Chunk* ArchetypeManager::CreateChunk(Archetype* a_pArchetype)
	{
		Chunk* _pChunk = new Chunk;
		_pChunk->pArchetype = a_pArchetype;
		_pChunk->count = 0;

		// メモリ確保
		_pChunk->entityData = new Entity[a_pArchetype->chunkCapacity];
		_pChunk->data = reinterpret_cast<uint8_t*>(
			operator new[](CHUNK_MEMORY_SIZE, std::align_val_t(a_pArchetype->maxAlign))
			);

		// ０初期化
		std::memset(_pChunk->data, 0, CHUNK_MEMORY_SIZE);

		a_pArchetype->chunks.push_back(_pChunk);

		// クエリのキャッシュはチャンク単位なので、チャンクが増えたら世代を進める
		m_generation++;

		return _pChunk;
	}

	void ArchetypeManager::CalcChunkLayout(Archetype* a_pArchetype, size_t a_memorySize)
	{
		const Signature& _sig = a_pArchetype->signature;

		//------------------------------------------------------------------
		// 並べるコンポーネントを集める(タイプID順 = 配列を並べる順)
		//------------------------------------------------------------------
		struct CompInfo
		{
			ComponentTypeID typeID;
			size_t			stride;		// 1要素のサイズ(配列の間隔)
			size_t			align;		// 配列の先頭に要るアライメント
		};
		std::vector<CompInfo> _compVec = {};

		size_t _entityStride = 0;		// 1エンティティが消費するバイト数(パディング抜き)
		size_t _maxAligne = 1;			// コンポーネントの最大アライメント
		for (ComponentTypeID _comTypeID = 0; _comTypeID < _sig.size(); ++_comTypeID)
		{
			// コンポーネント登録チェック
			if (!_sig.test(_comTypeID)) continue;

			const ComponentMeta& _meta = m_pMetaRegister->GetMetaData(_comTypeID);
			_compVec.push_back({ _comTypeID, _meta.compAlignSize, _meta.compAlign });

			_entityStride += _meta.compAlignSize;
			_maxAligne = std::max(_maxAligne, _meta.compAlign);
		}

		// 最大アライメント決定
		a_pArchetype->maxAlign = _maxAligne;

		// コンポーネントを1つも持たないアーキタイプは、エンティティ配列だけで容量が決まる
		if (_compVec.empty())
		{
			a_pArchetype->chunkCapacity = static_cast<uint32_t>(a_memorySize / sizeof(Entity));
			return;
		}

		//------------------------------------------------------------------
		// 容量の決定
		//------------------------------------------------------------------
		// 配列は SoA でコンポーネントごとに並べ、各配列の先頭をアライメントへ切り上げる
		// パディング抜きで割った値を上限にし、パディング込みで収まるまで減らす
		//------------------------------------------------------------------
		auto _calcRequiredSize = [&_compVec](size_t a_capacity)
			{
				size_t _size = 0;
				for (const CompInfo& _comp : _compVec)
				{
					_size = Math::Alignment::Up(_size, _comp.align);
					_size += _comp.stride * a_capacity;
				}
				return _size;
			};

		// 最大容量がメモリーサイズを下回るまで入るエンティティの数を１づつ減らしていく
		size_t _capacity = a_memorySize / _entityStride;
		while (_capacity > 0 && _calcRequiredSize(_capacity) > a_memorySize)
		{
			--_capacity;
		}

		// 1体ぶんすら入らない = コンポーネントが大きすぎる。
		// 容量0のまま進むと、割り当てのたびにチャンクを作って範囲外へ書き込む
		ENGINE_ERRLOG(_capacity > 0, "1エンティティのコンポーネントがチャンクのサイズを超えています");

		a_pArchetype->chunkCapacity = static_cast<uint32_t>(_capacity);

		//------------------------------------------------------------------
		// オフセット位置計算 : 容量の決定と同じ並べ方
		//------------------------------------------------------------------
		size_t _offset = 0;
		for (const CompInfo& _comp : _compVec)
		{
			_offset = Math::Alignment::Up(_offset, _comp.align);

			Layout _lay = {};
			_lay.offset = _offset;
			_lay.stride = _comp.stride;
			a_pArchetype->layoutMap.emplace(_comp.typeID, _lay);

			// 要素は stride 間隔で置くので、配列の長さも stride で数える
			_offset += _comp.stride * _capacity;
		}
	}

	void ArchetypeManager::ReleaseAllChunks()
	{
		for (auto& _upArchetype : m_upArchetypeVec)
		{
			for (Chunk* _pChunk : _upArchetype->chunks)
			{
				delete[] _pChunk->entityData;

				operator delete[](
					_pChunk->data,
					std::align_val_t(_upArchetype->maxAlign)
					);

				delete _pChunk;
			}
			_upArchetype->chunks.clear();
		}
	}
}
