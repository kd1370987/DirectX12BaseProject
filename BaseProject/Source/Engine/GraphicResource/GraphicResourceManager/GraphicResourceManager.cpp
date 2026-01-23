#include "GraphicResourceManager.h"

#include "../Loader/ModelLoader/TinyGLTFLoader/TinyGLTFLoader.h"
#include "../Loader/ModelLoader/AssimpLoader/AssimpLoader.h"

void GraphicResourceManager::Init()
{

	m_texStorage.Init(100);
	m_modelStorage.Init(100);
}

Resource::ID GraphicResourceManager::GetTexture(const std::string& a_key)
{
	if (!m_texStorage.Has(a_key))
	{
		Texture _tex;
		LoadTextureFromPath(_tex,a_key);
		return m_texStorage.Add(a_key, std::move(_tex));
	}
	else
	{
		return m_texStorage.GetID(a_key);
	}
}

const Texture* GraphicResourceManager::NGetTexture(const uint32_t& a_texID)
{
	return m_texStorage.Get(a_texID);
}

const Resource::ID& GraphicResourceManager::GetModel(const std::string& a_path)
{
	if (!m_modelStorage.Has(a_path))
	{
		Model _model = {};
		LoadModelFromPath(_model,a_path);
		return m_modelStorage.Add(a_path, std::move(_model));
	}
	else
	{
		return m_modelStorage.GetID(a_path);
	}
}

const Model* GraphicResourceManager::NGetModelResource(uint32_t a_modelID)
{
	return m_modelStorage.Get(a_modelID);
}

void GraphicResourceManager::LoadTextureFromPath(Texture& a_tex, const std::string& a_path)
{
	if (a_path.empty())
	{
		// パスがないのなら初期リソースを割り当てる
		a_tex.WhiteTexture();
	}
	else
	{
		// パスがあるのなら
		a_tex.NormalMapLoad(a_path);
	}
}

void GraphicResourceManager::LoadModelFromPath(Model& a_model, const std::string& a_path)
{
	//-------------------------------------
	// 対応形式チェック
	//-------------------------------------
	std::string _fileDir = FileUtility::GetDirFromPath(a_path);		// 親ディレクトリパス取得
	std::string _ext = FileUtility::GetFilePathExtension(a_path);	// 拡張子取得

	// 対応拡張子のファイルをディレクトリ内から全て取得
	auto _modelBinFile = FileUtility::FindExtensionInDirectory(_fileDir, ".modelBin");

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
			return ;
		}
		Serialize::TinyGLTF(a_model,_spGltfModel,_fileDir);
	}
	//-------------------------------------
	// Assimpを使用する場合
	//-------------------------------------
	else
	{
		auto _spAssimpModel = std::make_shared<AssimpModel>();
		std::string _filePath = a_path;
		AssimpLoader _loader;
		if (!_loader.Load(
			_filePath,
			*_spAssimpModel.get(),
			false,
			true
		))
		{
			assert(0 && "モデル読み込みに失敗\n");
			return;
		}

		Serialize::Assimp(a_model,_spAssimpModel,_fileDir);
	}
	// 読み込み成功
	return;
}
