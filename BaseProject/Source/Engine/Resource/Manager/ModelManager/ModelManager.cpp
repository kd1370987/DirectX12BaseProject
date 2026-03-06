#include "ModelManager.h"

#include "Engine/Graphics/GraphicResource/Loader/ModelLoader/TinyGLTFLoader/TinyGLTFLoader.h"

Engine::Resource::Handle<Engine::Resource::Model> Engine::Resource::ModelManager::LoadModel(
	const std::string& a_path
)
{
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
		// GLTFもしくはGLB形式のモデルデータを読み込む
		auto _spGltfModel = Load::Model(a_path);
		if (!_spGltfModel)
		{
			// 読み込み失敗
			assert(0 && "GLTFのシリアライズに失敗");
			return;
		}
		Serialize::TinyGLTF(_model, _spGltfModel, _fileDir);
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
	// 読み込み成功
	return Add(_model);
}

Engine::Resource::Handle<Engine::Resource::Model> Engine::Resource::ModelManager::LoadModel(
	std::string_view a_metaName
)
{
	return Engine::Resource::Handle<Engine::Resource::Model>();
}

const Engine::Resource::Model* Engine::Resource::ModelManager::GetModel(const Engine::Resource::Handle<Engine::Resource::Model>& a_handle)
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
