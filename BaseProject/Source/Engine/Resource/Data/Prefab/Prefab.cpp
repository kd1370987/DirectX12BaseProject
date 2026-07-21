#include "Prefab.h"

#include <filesystem>

#include "../../../ECS/World/World.h"
#include "../../../Scene/SceneManager/SceneManager.h"

#include "../../Manager/AssetDatabase/AssetDatabase.h"
#include "../../Manager/ResourceManager/ResourceManager.h"

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
		auto _dir = FileUtility::GetDirFromPath(a_savePath);
		auto _fileName = FileUtility::GetFileNameWithoutExtension(a_savePath);

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

		auto _dir = FileUtility::GetDirFromPath(a_filePath);
		auto _fileName = FileUtility::GetFileNameWithoutExtension(a_filePath);

		Persistence::Archive _arch(
			Persistence::Archive::Mode::Load, _dir, _fileName, "prfb",
			Persistence::Archive::ArchiveFormat::Json);

		Archive(_arch, a_pWorld);
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
	}
}
