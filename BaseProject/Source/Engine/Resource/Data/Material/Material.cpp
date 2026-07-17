#include "Material.h"

#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/D3D12//D3DObject/DescriptorHeap/DescriptorHeap.h"

//#include "Engine/Resource/Manager/TextureManager/TextureManager.h"
#include "Engine/Resource/Data/Texture/IO/TextureIO.h"
#include "../../Manager/ResourceManager/ResourceManager.h"

#include "../../../Utility/BinaryHelper/BinaryHelper.h"

#include "../../../Option/OptionManager.h"

void Engine::Resource::Material::Release()
{
	ENGINE_LOG("マテリアルの解放 : Release");
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

	baseColorTex	= TextureIO::LoadTexture(baseColorTexGUID, TexColor::WHITE);
	metaRoughTex	= TextureIO::LoadTexture(metaRoughTexGUID, TexColor::ORM);
	emissiveTex		= TextureIO::LoadTexture(emissiveTexGUID, TexColor::BLACK);
	normalTex		= TextureIO::LoadTexture(normalTexGUID, TexColor::NORMAL);


	shedingModelGUID = Option::OptionManager::GetInstance().GetRenderingOption().defaultShadingModelTable;
	shadingModelHandle = ResourceManager::Instance().Load<ShadingModelTable>(shedingModelGUID);
}

void Engine::Resource::Material::Archive(Persistence::Archive& a_ar)
{
	a_ar.StringField("MaterialName", name);
	a_ar.Field("AlphaMode", alphaMode);

	// 参照テクスチャGUID
	a_ar.Field("BaseColorTexGUID", baseColorTexGUID);
	a_ar.Field("MetaRoughTexGUID", metaRoughTexGUID);
	a_ar.Field("EmissiveTexGUID", emissiveTexGUID);
	a_ar.Field("NormalTexGUID", normalTexGUID);

	// スケール値
	a_ar.Field("BaseColor", baseColor);
	a_ar.Field("Metallic", metallic);
	a_ar.Field("Roughness", roughness);
	a_ar.Field("Emissive", emissive);

	// シェーディングモデル
	a_ar.Field("shedingModelGUID", shedingModelGUID);

	// もしシェーディングモデルがアセットとしてないタイプだった場合
	// デフォルトのシェーディングモデルを使用
	auto _path = AssetDatabase::Instance().GetFileNameFromGUID(shedingModelGUID);
	if (_path.empty())
	{
		shedingModelGUID = Option::OptionManager::GetInstance().GetRenderingOption().defaultShadingModelTable;
	}
}
