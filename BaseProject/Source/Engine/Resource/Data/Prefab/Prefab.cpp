#include "Prefab.h"


#include "../../../ECS/World/World.h"
#include "../../../Scene/SceneManager/SceneManager.h"

#include "../../Manager/AssetDatabase/AssetDatabase.h"
#include "../../Manager/ResourceManager/ResourceManager.h"

// 実体化のたびにGUIDを振り直すために触る。
// (World.h がフェーズタグを取り込んでいるのと同じで、ここも土台側のコンポーネント)
#include "Application/Components/Persistence/GUIDComponent.h"

namespace Engine::Resource
{
	Prefab::Prefab()
	{
		m_sigunature = {};
		m_dataMap.clear();
	}

	//======================================================================================
	// コンポーネント操作
	//======================================================================================
	void Prefab::AddComponentDefault(ECS::World* a_pWorld, ECS::ComponentTypeID a_compTypeID)
	{
		if (!a_pWorld) return;

		// コンポーネントのサイズ分バッファを確保して既定値で構築する
		const auto& _meta = a_pWorld->GetComponentMetaData(a_compTypeID);

		auto& _buffer = m_dataMap[a_compTypeID];
		_buffer.assign(_meta.compAlignSize, 0);

		auto _construct = a_pWorld->GetCompFunc(a_compTypeID).construct;
		if (_construct) _construct(_buffer.data());

		m_sigunature.set(a_compTypeID);
	}

	void Prefab::AddComponentData(ECS::World* a_pWorld, ECS::ComponentTypeID a_compTypeID, const uint8_t* a_pSrc)
	{
		if (!a_pWorld || !a_pSrc) return;

		// コンポーネント全体をコピーして保持する
		const auto& _meta = a_pWorld->GetComponentMetaData(a_compTypeID);

		auto& _buffer = m_dataMap[a_compTypeID];
		_buffer.assign(a_pSrc, a_pSrc + _meta.compAlignSize);

		m_sigunature.set(a_compTypeID);
	}

	void Prefab::RemoveComponent(ECS::ComponentTypeID a_compTypeID)
	{
		m_sigunature.reset(a_compTypeID);
		m_dataMap.erase(a_compTypeID);
	}

	void Prefab::PasteSignatureAndData(
		const ECS::Signature& a_sig,
		const std::unordered_map<ECS::ComponentTypeID, std::vector<uint8_t>>& a_dataMap)
	{
		// まるごと差し替える
		m_sigunature = a_sig;

		// データはシグネチャに載っているものだけを引き継ぐ。
		// (コピー元で削除済みのバッファが残っていても持ち込まないようにする)
		m_dataMap.clear();
		for (const auto& [_typeID, _buffer] : a_dataMap)
		{
			if (!a_sig.test(_typeID)) continue;
			if (_buffer.empty()) continue;

			m_dataMap[_typeID] = _buffer;
		}
	}

	bool Prefab::Has(ECS::ComponentTypeID a_compTypeID) const
	{
		return m_sigunature.test(a_compTypeID);
	}

	uint8_t* Prefab::RefData(ECS::ComponentTypeID a_compTypeID)
	{
		auto _it = m_dataMap.find(a_compTypeID);
		if (_it == m_dataMap.end() || _it->second.empty()) return nullptr;
		return _it->second.data();
	}

	//======================================================================================
	// 保存 / 読み込み
	//======================================================================================
	void Prefab::Save(ECS::World* a_pWorld, const std::string& a_savePath)
	{
		auto _dir = Engine::File::GetDirFromPath(a_savePath);
		auto _fileName = Engine::File::GetFileNameWithoutExtension(a_savePath);

		// 保存先ディレクトリが無ければ作成しておく
		if (!_dir.empty())
		{
			std::error_code _ec;
			std::filesystem::create_directories(_dir, _ec);
		}

		Persistence::Archive _arch(Persistence::Archive::Mode::Save, _dir, _fileName, "prfb");
		Archive(_arch, a_pWorld);
	}

	void Prefab::Load(ECS::World* a_pWorld, const std::string& a_filePath)
	{
		// 一旦クリアしてから読み込む
		m_sigunature = {};
		m_dataMap.clear();

		auto _dir = Engine::File::GetDirFromPath(a_filePath);
		auto _fileName = Engine::File::GetFileNameWithoutExtension(a_filePath);

		Persistence::Archive _arch(
			Persistence::Archive::Mode::Load, _dir, _fileName, "prfb",
			Persistence::Archive::ArchiveFormat::Json);

		Archive(_arch, a_pWorld);
	}

	//======================================================================================
	// 実体化の材料を組み立てる
	//--------------------------------------------------------------------------------------
	// ノードごとに新しいGUIDを振り、保存時GUIDへの参照を全部そちらへ張り替える。
	//
	// 張り替えはバイト列を GUID(16バイト)単位で走査して行う。
	// 参照を持つコンポーネント(親リンク・アタッチメント・ターゲット指定など)を
	// ここで型として列挙すると、参照を持つコンポーネントが増えるたびに
	// このリソースを直す羽目になる。GUID は乱数なので、保存したエンティティの
	// GUID と偶然一致するバイト列が別の意味で入っていることは実質ない。
	// アセットのGUID(モデルや音)は対応表に載っていないので触られない。
	//======================================================================================
	std::vector<PrefabInstanceData> Prefab::BuildInstanceData(ECS::World* a_pWorld) const
	{
		std::vector<PrefabInstanceData> _instanceVec = {};
		if (!a_pWorld) return _instanceVec;

		_instanceVec.reserve(m_children.size() + 1);

		// ---- ルート ----
		PrefabInstanceData _root = {};
		_root.sig     = m_sigunature;
		_root.dataMap = m_dataMap;
		_instanceVec.push_back(std::move(_root));

		// ---- 子 ----
		for (const PrefabChild& _child : m_children)
		{
			PrefabInstanceData _data = {};
			_data.sig     = _child.sig;
			_data.dataMap = _child.dataMap;
			_instanceVec.push_back(std::move(_data));
		}

		//----------------------------------------------------------------------
		// 保存時GUID → 新しいGUID の対応表を作る
		//----------------------------------------------------------------------
		std::unordered_map<Engine::GUID, Engine::GUID> _guidMap = {};

		std::vector<Engine::GUID> _newGUIDVec(_instanceVec.size());
		for (auto& _newGUID : _newGUIDVec) _newGUID.Create();

		if (m_savedGUID.IsValid()) _guidMap[m_savedGUID] = _newGUIDVec[0];

		for (size_t _i = 0; _i < m_children.size(); ++_i)
		{
			const Engine::GUID& _savedGUID = m_children[_i].savedGUID;
			if (!_savedGUID.IsValid()) continue;

			_guidMap[_savedGUID] = _newGUIDVec[_i + 1];
		}

		//----------------------------------------------------------------------
		// 参照の張り替えと、自分自身のGUIDの書き込み
		//----------------------------------------------------------------------
		const ECS::ComponentTypeID _guidTypeID = a_pWorld->GetCompTypeID<GUIDComponent>();

		for (size_t _i = 0; _i < _instanceVec.size(); ++_i)
		{
			for (auto& [_typeID, _buffer] : _instanceVec[_i].dataMap)
			{
				if (_buffer.empty()) continue;
				RemapGUIDs(_buffer.data(), _buffer.size(), _guidMap);
			}

			// 自分のGUIDは対応表に載っていない場合(ルートは空で保存している)があるので、
			// ここで確実に新しいものを入れておく。子から親を引けるようにするため
			if (_guidTypeID == ECS::Limits::INVALID_COMPONENTTYPEID) continue;
			if (!_instanceVec[_i].sig.test(_guidTypeID)) continue;

			auto _it = _instanceVec[_i].dataMap.find(_guidTypeID);
			if (_it == _instanceVec[_i].dataMap.end() || _it->second.size() < sizeof(GUIDComponent))
			{
				continue;
			}

			GUIDComponent _guidComp = {};
			std::memcpy(&_guidComp, _it->second.data(), sizeof(_guidComp));
			_guidComp.guid = _newGUIDVec[_i];
			std::memcpy(_it->second.data(), &_guidComp, sizeof(_guidComp));
		}

		return _instanceVec;
	}

	//======================================================================================
	// バイト列に残っている GUID を張り替える
	//--------------------------------------------------------------------------------------
	// GUID(UUID)の先頭は 4 バイト境界に載るので、4 バイト刻みで見れば取りこぼさない。
	//======================================================================================
	void Prefab::RemapGUIDs(
		uint8_t* a_pData, size_t a_size,
		const std::unordered_map<Engine::GUID, Engine::GUID>& a_guidMap)
	{
		if (!a_pData || a_guidMap.empty()) return;
		if (a_size < sizeof(Engine::GUID)) return;

		constexpr size_t _kStride = 4;
		const size_t _end = a_size - sizeof(Engine::GUID);

		for (size_t _offset = 0; _offset <= _end; _offset += _kStride)
		{
			Engine::GUID _guid = {};
			std::memcpy(&_guid, a_pData + _offset, sizeof(_guid));

			// 未設定(全ゼロ)は「参照なし」の意味なので触らない
			if (!_guid.IsValid()) continue;

			auto _it = a_guidMap.find(_guid);
			if (_it == a_guidMap.end()) continue;

			std::memcpy(a_pData + _offset, &_it->second, sizeof(_it->second));
		}
	}

	//======================================================================================
	// 実体化
	//======================================================================================
	ECS::Entity Prefab::Instantiate(ECS::World* a_pWorld)
	{
		if (!a_pWorld) return ECS::Limits::INVALID_ENTITY;

		// ルート + 子ぶんの材料(GUIDは振り直し済み)
		std::vector<PrefabInstanceData> _instanceVec = BuildInstanceData(a_pWorld);
		if (_instanceVec.empty()) return ECS::Limits::INVALID_ENTITY;

		ECS::Entity _rootEntity = ECS::Limits::INVALID_ENTITY;

		for (size_t _i = 0; _i < _instanceVec.size(); ++_i)
		{
			PrefabInstanceData& _data = _instanceVec[_i];

			// シグネチャで実体を生成(各コンポーネントは既定構築される)
			ECS::Entity _entity = a_pWorld->CreateEntity(_data.sig);
			if (_entity == ECS::Limits::INVALID_ENTITY) continue;

			if (_i == 0) _rootEntity = _entity;

			// 保存済みの初期値を各コンポーネントへ流し込む
			// (ハンドル等のランタイム値は既定のまま。GUID から PostDeserialize 系システムが復元する。
			//  親子リンクも parentGUID を張り替えてあるので HierarchyLinkSystem が繋ぎ直す)
			for (auto& [_typeID, _buffer] : _data.dataMap)
			{
				if (_buffer.empty()) continue;
				if (!_data.sig.test(_typeID)) continue;

				uint8_t* _dst = a_pWorld->NRefData(_entity, _typeID);
				if (!_dst) continue;

				size_t _size = a_pWorld->GetComponentMetaData(_typeID).compSize;
				std::memcpy(_dst, _buffer.data(), _size);
			}
		}

		return _rootEntity;
	}

	//======================================================================================
	// ローダー / 生成
	//======================================================================================
	Prefab Prefab::LoadFromFile(const std::string& a_path)
	{
		Prefab _prefab;

		// コンポーネントのメタ情報が必要なので World を取得する
		ECS::World* _pWorld = Scene::SceneManager::Instance().RefWorld();
		if (_pWorld && _pWorld->IsInit())
		{
			_prefab.Load(_pWorld, a_path);
		}

		return _prefab;
	}

	void Prefab::Create(const std::string& a_path, const std::string& a_name)
	{
		static std::string _dir = "Asset/Prefab/";
		auto _basePath = _dir + a_path + "/" + a_name;

		// すでに存在するなら作らない
		Engine::GUID _checkGUID = AssetDatabase::Instance().GetGUIDFromFilePath(_basePath);
		if (_checkGUID != Engine::DefaultGUID)
		{
			ENGINE_LOG("すでに作成されたプレハブです : %s", _basePath.c_str());
			return;
		}

		// メタ情報を登録して GUID を発行
		auto _guid = AssetDatabase::Instance().AddMetaData(_basePath, "Prefab");

		// 空のプレハブを保存してプールに登録
		Prefab _prefab;
		ECS::World* _pWorld = Scene::SceneManager::Instance().RefWorld();
		_prefab.Save(_pWorld, _basePath);

		ResourceManager::Instance().AddResourceAndGUID(std::move(_prefab), _guid);
	}

	//======================================================================================
	// シリアライズ処理
	//======================================================================================
	void Prefab::Archive(Persistence::Archive& a_ar, ECS::World* a_pWorld)
	{
		std::vector<std::string> _compNames = {};

		// 【セーブ時のみ】シグネチャからコンポーネント名リストを作成
		if (a_ar.GetMode() == Persistence::Archive::Mode::Save)
		{
			if (a_pWorld)
			{
				for (auto& [_typeID, _meta] : a_pWorld->GetAllComponentMetaData())
				{
					if (m_sigunature.test(_typeID))
					{
						_compNames.push_back(_meta.name);
					}
				}
			}
		}

		// コンポーネント名リストのアーカイブ
		a_ar.VectorField("ComponentNames", _compNames);

		// 【ロード時のみ】読み込んだ名前からシグネチャを作り、既定値バッファを確保
		if (a_ar.GetMode() == Persistence::Archive::Mode::Load)
		{
			if (!a_pWorld) return;

			for (const std::string& _name : _compNames)
			{
				ECS::ComponentTypeID _typeID = a_pWorld->GetCompTypeID(_name);
				if (_typeID == ECS::Limits::INVALID_COMPONENTTYPEID) continue;

				// バッファ確保 + 既定構築(このあと各データで上書きされる)
				AddComponentDefault(a_pWorld, _typeID);
			}
		}

		// ---------------------------------------------------------
		// 各コンポーネントデータのシリアライズ
		// ---------------------------------------------------------
		if (!a_pWorld) return;

		for (const std::string& _name : _compNames)
		{
			ECS::ComponentTypeID _typeID = a_pWorld->GetCompTypeID(_name);
			if (_typeID == ECS::Limits::INVALID_COMPONENTTYPEID) continue;

			auto _it = m_dataMap.find(_typeID);
			if (_it == m_dataMap.end() || _it->second.empty()) continue;

			auto _func = a_pWorld->GetCompFunc(_typeID).archive;
			if (_func)
			{
				// セーブもロードも同じグループ構造で実行
				if (a_ar.BeginGroup(_name))
				{
					_func(a_ar, _it->second.data());
					a_ar.EndGroup();
				}
			}
		}

		//----------------------------------------------------------------------
		// ルートのGUIDと子エンティティ
		//----------------------------------------------------------------------
		// 既存のプレハブにはこれらのキーが無い。読み込み時は BeginArray が false を返し、
		// GUID も既定のまま残るので、子なしのプレハブとして今までどおり動く。
		//----------------------------------------------------------------------
		a_ar.GUIDField("SavedGUID", m_savedGUID);

		size_t _childCount = m_children.size();
		if (!a_ar.BeginArray("Children", _childCount)) return;

		m_children.resize(_childCount);

		for (size_t _i = 0; _i < _childCount; ++_i)
		{
			if (!a_ar.BeginObject(_i)) continue;

			PrefabChild& _child = m_children[_i];

			a_ar.GUIDField("SavedGUID", _child.savedGUID);
			a_ar.Field("ParentIndex", _child.parentIndex);

			// この子が持つコンポーネント名
			std::vector<std::string> _childCompNames = {};
			if (a_ar.GetMode() == Persistence::Archive::Mode::Save)
			{
				for (auto& [_typeID, _meta] : a_pWorld->GetAllComponentMetaData())
				{
					if (_child.sig.test(_typeID)) _childCompNames.push_back(_meta.name);
				}
			}
			a_ar.VectorField("ComponentNames", _childCompNames);

			// 【ロード時のみ】名前からシグネチャを作り、既定値バッファを確保する
			if (a_ar.GetMode() == Persistence::Archive::Mode::Load)
			{
				_child.sig = {};
				_child.dataMap.clear();

				for (const std::string& _name : _childCompNames)
				{
					ECS::ComponentTypeID _typeID = a_pWorld->GetCompTypeID(_name);
					if (_typeID == ECS::Limits::INVALID_COMPONENTTYPEID) continue;

					_child.sig.set(_typeID);

					auto& _buffer = _child.dataMap[_typeID];
					_buffer.assign(a_pWorld->GetComponentMetaData(_typeID).compAlignSize, 0);

					auto _ctor = a_pWorld->GetCompFunc(_typeID).construct;
					if (_ctor) _ctor(_buffer.data());
				}
			}

			// 各コンポーネントデータ(ルートと同じ形)
			for (const std::string& _name : _childCompNames)
			{
				ECS::ComponentTypeID _typeID = a_pWorld->GetCompTypeID(_name);
				if (_typeID == ECS::Limits::INVALID_COMPONENTTYPEID) continue;

				auto _dataIt = _child.dataMap.find(_typeID);
				if (_dataIt == _child.dataMap.end() || _dataIt->second.empty()) continue;

				auto _func = a_pWorld->GetCompFunc(_typeID).archive;
				if (!_func) continue;

				if (a_ar.BeginGroup(_name))
				{
					_func(a_ar, _dataIt->second.data());
					a_ar.EndGroup();
				}
			}

			a_ar.EndObject();
		}
		a_ar.EndArray();
	}
}
