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

		_ar.StringField("MaterialName", _mat.name);
		_ar.Field("AlphaMode", _mat.alphaMode);

		// 参照テクスチャGUID
		_ar.Field("BaseColorTexGUID", _mat.baseColorTexGUID);
		_ar.Field("MetaRoughTexGUID", _mat.metaRoughTexGUID);
		_ar.Field("EmissiveTexGUID", _mat.emissiveTexGUID);
		_ar.Field("NormalTexGUID", _mat.normalTexGUID);

		// スケール値
		_ar.Field("BaseColor", _mat.baseColor);
		_ar.Field("Metallic", _mat.metallic);
		_ar.Field("Roughness", _mat.roughness);
		_ar.Field("Emissive", _mat.emissive);

		// 読み込み
		_mat.baseColorTex = TextureLoader::LoadTexture(_mat.baseColorTexGUID, TexColor::WHITE);
		_mat.metaRoughTex = TextureLoader::LoadTexture(_mat.metaRoughTexGUID, TexColor::ORM);
		_mat.emissiveTex = TextureLoader::LoadTexture(_mat.emissiveTexGUID, TexColor::BLACK);
		_mat.normalTex = TextureLoader::LoadTexture(_mat.normalTexGUID, TexColor::NORMAL);

		return _mat;
	}

}


