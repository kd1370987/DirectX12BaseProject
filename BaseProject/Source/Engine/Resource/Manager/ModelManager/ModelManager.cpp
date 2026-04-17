#include "ModelManager.h"

#include "Engine/Resource/Importer/Model/ModelImporter.h"

Engine::Resource::Handle<Engine::Resource::Model> Engine::Resource::ModelManager::LoadModel(
	const std::string& a_path
)
{
	
	auto _it = m_handleMap.find(a_path);
	if (_it != m_handleMap.end())
	{
		return _it->second;
	}

	//-------------------------------------
	// 対応形式チェック
	//-------------------------------------
	std::string _fileDir = FileUtility::GetDirFromPath(a_path);		// 親ディレクトリパス取得
	std::string _ext = FileUtility::GetFilePathExtension(a_path);	// 拡張子取得

	// 対応拡張子のファイルをディレクトリ内から全て取得
	auto _modelBinFile = FileUtility::FindExtensionInDirectory(_fileDir, ".modelBin");

	// モデル
	Engine::Resource::Model _model = {};

	//-------------------------------------
	// 独自の形式があった場合
	//-------------------------------------
	if (_modelBinFile.size() > 0)
	{
		// モデルクラスをバイナリ化したデータを読み込む
		assert(0 && "独自の読み込みは未対応");
	}
	//-------------------------------------
	// TinyGLTFを使用する場合
	//-------------------------------------
	else if (_ext == "gltf" || _ext == "glb")
	{
		_model = ImportModel(a_path);
	}
	//-------------------------------------
	// Assimpを使用する場合
	//-------------------------------------
	else
	{
		//auto _spAssimpModel = std::make_shared<AssimpModel>();
		//std::string _filePath = a_path;
		//AssimpLoader _loader;
		//if (!_loader.Load(
		//	_filePath,
		//	*_spAssimpModel.get(),
		//	false,
		//	true
		//))
		//{
		//	assert(0 && "モデル読み込みに失敗\n");
		//	return;
		//}

		//Serialize::Assimp(a_model, _spAssimpModel, _fileDir);
	}

	// 一時的にパスを名前として使用
	_model.name = a_path;

	// 読み込み成功
	auto _handle = Add(_model);
	m_handleMap.emplace(a_path,_handle);
	return _handle;
}

const Engine::Resource::Model* Engine::Resource::ModelManager::GetModel(const Engine::Resource::Handle<Engine::Resource::Model>& a_handle)
{
	return RefModel(a_handle);
}

Engine::Resource::Model* Engine::Resource::ModelManager::RefModel(const Engine::Resource::Handle<Engine::Resource::Model>& a_handle)
{
	// インデックスサイズチェック
	if (a_handle.idx >= m_slotStorage.size())
		return nullptr;
	// 世代チェック
	if (m_slotStorage[a_handle.idx].gen != a_handle.gen)
		return nullptr;

	// 問題なければデータを返す
	return &m_slotStorage[a_handle.idx].data;
}

const Engine::Resource::Handle<Engine::Resource::Model>& Engine::Resource::ModelManager::GetHandle(const std::string& a_name)
{
	auto _it = m_handleMap.find(a_name);
	if (_it != m_handleMap.end())
	{
		return _it->second;
	}
	return Engine::Resource::Handle<Model>();
}

std::vector<Engine::Resource::SharedSlot<Engine::Resource::Model>>& Engine::Resource::ModelManager::GetAllModel()
{
	return m_slotStorage;
}

Engine::Resource::Handle<Engine::Resource::Model> Engine::Resource::ModelManager::Add(const Model& a_model)
{
	// キューが空なら、サイズを広げる
	if(m_indexQueue.empty())
	{
		m_indexQueue.push(m_indexQueueMaxSize);
		m_indexQueueMaxSize++;
	}

	// キューからインデックスをもらう
	Engine::Resource::Index _idx = m_indexQueue.front();
	m_indexQueue.pop();

	// ストレージに追加
	if (_idx >= m_slotStorage.size())
	{
		m_slotStorage.resize(_idx + 1);
	}
	m_slotStorage[_idx].data = a_model;
	m_slotStorage[_idx].gen = 0;
	m_slotStorage[_idx].sharedCount = 1;

	// ハンドルを作成して返す
	Engine::Resource::Handle<Engine::Resource::Model> _handle = {};
	_handle.gen = 0;
	_handle.idx = _idx;

	return _handle;
}

void Engine::Resource::ModelManager::Subtract(const Engine::Resource::Handle<Engine::Resource::Model>& a_handle)
{
	// 安全チェック
	if (a_handle.idx >= m_slotStorage.size())
		return;
	if (m_slotStorage[a_handle.idx].gen != a_handle.gen)
		return;

	// 空のデータを入れる
	m_slotStorage[a_handle.idx].data = Engine::Resource::Model{};
	m_slotStorage[a_handle.idx].gen++;

	// インデックスをキューに返還
	m_indexQueue.push(a_handle.idx);
}

Engine::Resource::ModelManager::ModelManager()
{
}

Engine::Resource::ModelManager::~ModelManager()
{
}
