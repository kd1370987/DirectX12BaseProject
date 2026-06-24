#include "Material.h"

#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/D3D12//D3DObject/DescriptorHeap/DescriptorHeap.h"

//#include "Engine/Resource/Manager/TextureManager/TextureManager.h"
#include "Engine/Resource/Loader/Texture/TextureLoader.h"
#include "../../Manager/ResourceManager/ResourceManager.h"

#include "../../../Utility/BinaryHelper/BinaryHelper.h"

void Engine::Resource::Material::Release()
{

}

void Engine::Resource::Material::SetTexture2D(
	const std::string& a_fileDir,
	const std::string& a_baseColorTexFileName,
	const std::string& a_metallicRoughnessTexFileName,
	const std::string& a_emissiveTexFileName,
	const std::string& a_normalTexFileName
)
{
	baseColorTexGUID	= AssetDatabase::Instance().GetGUIDFromFilePath(a_fileDir + a_baseColorTexFileName);
	metaRoughTexGUID	= AssetDatabase::Instance().GetGUIDFromFilePath(a_fileDir + a_metallicRoughnessTexFileName);
	emissiveTexGUID		= AssetDatabase::Instance().GetGUIDFromFilePath(a_fileDir + a_emissiveTexFileName);
	normalTexGUID		= AssetDatabase::Instance().GetGUIDFromFilePath(a_fileDir + a_normalTexFileName);

	baseColorTex	= TextureLoader::LoadTexture(baseColorTexGUID, TexColor::WHITE);
	metaRoughTex	= TextureLoader::LoadTexture(metaRoughTexGUID, TexColor::ORM);
	emissiveTex		= TextureLoader::LoadTexture(emissiveTexGUID, TexColor::BLACK);
	normalTex		= TextureLoader::LoadTexture(normalTexGUID, TexColor::NORMAL);
}

void Engine::Resource::Material::Save(const std::string& a_fileDir, const std::string& a_name)
{
	Persistence::Archive _ar(Persistence::Archive::Mode::Save,a_fileDir, a_name, "mtrl");
	_ar.StringField("MaterialName", name);
	_ar.Field("AlphaMode", alphaMode);

	// 参照テクスチャGUID
	_ar.Field("BaseColorTexGUID",baseColorTexGUID);
	_ar.Field("MetaRoughTexGUID", metaRoughTexGUID);
	_ar.Field("EmissiveTexGUID", emissiveTexGUID);
	_ar.Field("NormalTexGUID", normalTexGUID);

	// スケール値
	_ar.Field("BaseColor",baseColor);
	_ar.Field("Metallic", metallic);
	_ar.Field("Roughness", roughness);
	_ar.Field("Emissive", emissive);
}

void Engine::Resource::Material::Load(const std::string& a_fileDir, const std::string& a_name)
{
	Persistence::Archive _ar(Persistence::Archive::Mode::Load,a_fileDir, a_name, "mtrl");
	_ar.StringField("MaterialName", name);
	_ar.Field("AlphaMode", alphaMode);

	// 参照テクスチャGUID
	_ar.Field("BaseColorTexGUID", baseColorTexGUID);
	_ar.Field("MetaRoughTexGUID", metaRoughTexGUID);
	_ar.Field("EmissiveTexGUID", emissiveTexGUID);
	_ar.Field("NormalTexGUID", normalTexGUID);

	// スケール値
	_ar.Field("BaseColor", baseColor);
	_ar.Field("Metallic", metallic);
	_ar.Field("Roughness", roughness);
	_ar.Field("Emissive", emissive);

	// 読み込み
	baseColorTex	= TextureLoader::LoadTexture(baseColorTexGUID, TexColor::WHITE);
	metaRoughTex	= TextureLoader::LoadTexture(metaRoughTexGUID, TexColor::ORM);
	emissiveTex		= TextureLoader::LoadTexture(emissiveTexGUID, TexColor::BLACK);
	normalTex		= TextureLoader::LoadTexture(normalTexGUID, TexColor::NORMAL);
}
