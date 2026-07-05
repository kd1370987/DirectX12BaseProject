#include "MaterialLoader.h"
#include "../../Manager/ResourceManager/ResourceManager.h"
namespace Engine::Resource
{
	Material MaterialLoader::LoadFromFile(const std::string& a_path)
	{
		Material _mat = {};
		auto _fileDir = FileUtility::GetDirFromPath(a_path);
		auto _fileName = FileUtility::GetFileNameWithoutExtension(a_path);
		Persistence::Archive _ar(Persistence::Archive::Mode::Load, _fileDir, _fileName, "mtrl");
		_mat.Archive(_ar);

		// 読み込み
		_mat.baseColorTex = TextureLoader::LoadTexture(_mat.baseColorTexGUID, TexColor::WHITE);
		_mat.metaRoughTex = TextureLoader::LoadTexture(_mat.metaRoughTexGUID, TexColor::ORM);
		_mat.emissiveTex = TextureLoader::LoadTexture(_mat.emissiveTexGUID, TexColor::BLACK);
		_mat.normalTex = TextureLoader::LoadTexture(_mat.normalTexGUID, TexColor::NORMAL);

		_mat.shadingModelHandle = ResourceManager::Instance().Load<ShadingModelTable>(_mat.shedingModelGUID);

		return _mat;
	}

}