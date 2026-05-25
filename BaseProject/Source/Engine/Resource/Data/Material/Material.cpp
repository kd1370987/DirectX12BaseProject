#include "Material.h"

#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/D3D12//D3DObject/DescriptorHeap/DescriptorHeap.h"

//#include "Engine/Resource/Manager/TextureManager/TextureManager.h"
#include "Engine/Resource/Loader/Texture/TextureLoader.h"

#include "../../../Utility/BinaryHelper/BinaryHelper.h"

void Engine::Resource::Material::SetTexture2D(
	const std::string& a_fileDir,
	const std::string& a_baseColorTexFileName,
	const std::string& a_metallicRoughnessTexFileName,
	const std::string& a_emissiveTexFileName,
	const std::string& a_normalTexFileName
)
{
	baseColorTexGUID = Engine::Resource::TextureLoader::RequestGUID(a_fileDir + a_baseColorTexFileName, TexColor::WHITE);
	metaRoughTexGUID = Engine::Resource::TextureLoader::RequestGUID(a_fileDir + a_metallicRoughnessTexFileName, { 0,255,255,255 });
	emissiveTexGUID = Engine::Resource::TextureLoader::RequestGUID(a_fileDir + a_emissiveTexFileName, { 0,0,0,255 });
	normalTexGUID = Engine::Resource::TextureLoader::RequestGUID(a_fileDir + a_normalTexFileName, { 128,128,255,255 });

	baseColorTex = Engine::Resource::TextureLoader::Request(a_fileDir + a_baseColorTexFileName,TexColor::WHITE);
	metaRoughTex = Engine::Resource::TextureLoader::Request(a_fileDir + a_metallicRoughnessTexFileName, { 0,255,255,255 });
	emissiveTex = Engine::Resource::TextureLoader::Request(a_fileDir + a_emissiveTexFileName, { 0,0,0,255 });
	normalTex = Engine::Resource::TextureLoader::Request(a_fileDir + a_normalTexFileName, { 128,128,255,255 });
}

void Engine::Resource::Material::Save(const std::string& a_fileDir, const std::string& a_name)
{
	Persistence::Archive _ar(Persistence::Archive::Mode::Save,a_fileDir, a_name, "mtrl");
	_ar.StringField("MaterialName", name);
	_ar.Field("AlphaMode", alphaMode);

	// 参照テクスチャGUID
	_ar.GUIDField("BaseColorTexGUID",baseColorTexGUID);
	_ar.GUIDField("MetaRoughTexGUID", metaRoughTexGUID);
	_ar.GUIDField("EmissiveTexGUID", emissiveTexGUID);
	_ar.GUIDField("NormalTexGUID", normalTexGUID);

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
	_ar.GUIDField("BaseColorTexGUID", baseColorTexGUID);
	_ar.GUIDField("MetaRoughTexGUID", metaRoughTexGUID);
	_ar.GUIDField("EmissiveTexGUID", emissiveTexGUID);
	_ar.GUIDField("NormalTexGUID", normalTexGUID);

	// スケール値
	_ar.Field("BaseColor", baseColor);
	_ar.Field("Metallic", metallic);
	_ar.Field("Roughness", roughness);
	_ar.Field("Emissive", emissive);

	// 読み込み
	baseColorTex	= Engine::Resource::TextureLoader::Load(baseColorTexGUID, TexColor::WHITE);
	metaRoughTex	= Engine::Resource::TextureLoader::Load(metaRoughTexGUID, TexColor::ORM);
	emissiveTex		= Engine::Resource::TextureLoader::Load(emissiveTexGUID, TexColor::BLACK);
	normalTex		= Engine::Resource::TextureLoader::Load(normalTexGUID, TexColor::NORMAL);
}
