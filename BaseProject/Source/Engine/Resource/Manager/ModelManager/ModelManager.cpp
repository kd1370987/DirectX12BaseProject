#include "ModelManager.h"

#include "Engine/Resource/Importer/Model/ModelImporter.h"

#include "../AssetManager/AssetManager.h"

namespace Engine::Resource
{
	void ModelManager::Init(AssetManager* a_pAssetManager)
	{
		m_pAssetManager = a_pAssetManager;
	}
	
	Handle<Model> ModelManager::Load(const Engine::GUID& a_guid)
	{
		// 登録されているかのチェック
		if (Has(a_guid))
		{
			return m_guidToModelHandleMap[a_guid];
		}

		// パスの取得
		auto _path = m_pAssetManager->GetFilePathFromGUID(a_guid);

		// モデルの作成
		Model _model = {};
		_model.Import(_path);

		// モデルを追加とハンドルをアロケート
		auto _handle = m_handleStorage.Allocate();
		if (_handle.idx >= m_modelVec.size())
		{
			m_modelVec.resize(_handle.idx + 1);
		}
		m_modelVec[_handle.idx] = _model;

		// 対応表を作成
		m_guidToModelHandleMap[a_guid] = _handle;

		return _handle;
	}

	
	const Model* ModelManager::GetModel(const Handle<Model>& a_handle)
	{
		return RefModel(a_handle);
	}

	Model* ModelManager::RefModel(const Handle<Model>& a_handle)
	{
		// 世代確認
		if (m_handleStorage.IsValid(a_handle))
		{
			return &m_modelVec[a_handle.idx];
		}

		return nullptr;
	}

	const Handle<Model>& ModelManager::GetHandle(const Engine::GUID& a_guid)
	{
		if (Has(a_guid))
		{
			return m_guidToModelHandleMap[a_guid];
		}
		return {};
	}

	const std::unordered_map<Engine::GUID, Handle<Model>> ModelManager::GetAllModelHandleMap()
	{
		return m_guidToModelHandleMap;
	}

	const std::vector<Model>& ModelManager::GetAllModel() const
	{
		return m_modelVec;
	}

	bool ModelManager::Has(const Engine::GUID& a_guid) const
	{
		auto _it = m_guidToModelHandleMap.find(a_guid);
		if (_it != m_guidToModelHandleMap.end())
		{
			return true;
		}

		return false;
	}

	ModelManager::ModelManager()
	{}

	ModelManager::~ModelManager()
	{}
}