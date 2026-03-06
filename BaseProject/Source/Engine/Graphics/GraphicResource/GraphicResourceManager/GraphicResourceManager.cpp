#include "GraphicResourceManager.h"

#include "../Loader/ModelLoader/TinyGLTFLoader/TinyGLTFLoader.h"
#include "../Loader/ModelLoader/AssimpLoader/AssimpLoader.h"

#include "../Loader/TextureLoader/TextureLoader.h"

void GraphicResourceManager::Init()
{

	m_texStorage.Init(100);
	m_modelStorage.Init(100);
}

void GraphicResourceManager::Release()
{
	m_texStorage.Clear();
	m_modelStorage.Clear();
}

bool GraphicResourceManager::GetTexture(Engine::Resource::ID& a_outID, const std::string& a_dir, const std::string& a_key, TextureUse a_use)
{
	ImGuiContex::Instance().AddLog("GetTexture: %s\n", (a_dir + a_key).c_str());

	if (a_key == "")
	{
		switch (a_use)
		{
		case TextureUse::Albedo:
			a_outID = m_texStorage.Add(a_dir + "Albedo", TextureLoad::White());
			return false;
			break;
		case TextureUse::MetallicRoughness:
			a_outID = m_texStorage.Add(a_dir + "MetaRoug", TextureLoad::ORM());
			return false;
		case TextureUse::Emissive:
			a_outID = m_texStorage.Add(a_dir + "Emissive", TextureLoad::Black());
			return false;
		case TextureUse::Normal:
			a_outID =  m_texStorage.Add(a_dir + "Normal", TextureLoad::NormalWhite());
			return false;

		default:
			break;
		}
	}

	std::string _fullPath = a_dir + a_key;

	if (!m_texStorage.Has(_fullPath))
	{
		Texture _tex = {};
		if (!TextureLoad::Load(_fullPath, _tex))
		{
			switch (a_use)
			{
			case TextureUse::Albedo:
				_tex = TextureLoad::White();
				break;
			case TextureUse::MetallicRoughness:
				_tex = TextureLoad::ORM();
				break;
			case TextureUse::Emissive:
				_tex = TextureLoad::Black();
				break;
			case TextureUse::Normal:
				_tex = TextureLoad::NormalWhite();
				break;
			default:
				break;
			}
		}
		a_outID = m_texStorage.Add(_fullPath, _tex);
		return true;
	}
	else
	{
		a_outID = m_texStorage.GetID(_fullPath);
		return true;
	}
}

const Texture* GraphicResourceManager::NGetTexture(const uint32_t& a_texID)
{
	return m_texStorage.Get(a_texID);
}

const std::string& GraphicResourceManager::GetTexturePath(const uint32_t& a_texID)
{
	return m_texStorage.GetString(a_texID);
}

Engine::Resource::ID GraphicResourceManager::GetModel(const std::string& a_path)
{
	if (!m_modelStorage.Has(a_path))
	{
		Engine::Resource::Model _model = {};
		LoadModelFromPath(_model,a_path);
		return m_modelStorage.Add(a_path, _model);
	}
	else
	{
		return m_modelStorage.GetID(a_path);
	}
}

const Engine::Resource::Model* GraphicResourceManager::NGetModelResource(uint32_t a_modelID)
{
	return m_modelStorage.Get(a_modelID);
}

Engine::Resource::Model* GraphicResourceManager::NGetModel(uint32_t a_modelID)
{
	return m_modelStorage.Ref(a_modelID);
}

UINT GraphicResourceManager::GetModelResourceStorageSize()
{
	return static_cast<UINT>(m_modelStorage.GetSize());
}

void GraphicResourceManager::LoadModelFromPath(Engine::Resource::Model& a_model, const std::string& a_path)
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
