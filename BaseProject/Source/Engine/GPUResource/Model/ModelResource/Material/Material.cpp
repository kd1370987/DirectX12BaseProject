#include "Material.h"

#include "Engine/GPUResource/Texture/Texture.h"

#include "Engine/Graphics/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/GPUResource/DescriptorHeap/DescriptorHeap.h"
#include "Engine/ResourceManager/ResourceManager.h"

void Material::SetTexture2D(
	const std::string& a_fileDir, 
	const std::string& a_baseColorTexFileName, 
	const std::string& a_metallicRoughnessTexFileName, 
	const std::string& a_emissiveTexFileName,
	const std::string& a_normalTexFileName
)
{
	// 基本色テクスチャ
	if (!a_baseColorTexFileName.empty() && FileUtility::IsExistFile(a_fileDir + a_baseColorTexFileName))
	{
		auto _wpTex = ResourceManager::Instance().GetTexture(a_fileDir + a_baseColorTexFileName);
		baseColorTexKey = a_fileDir + a_baseColorTexFileName;
	}
	// メタリック・ラフネステクスチャ
	if (!a_metallicRoughnessTexFileName.empty() && FileUtility::IsExistFile(a_fileDir + a_metallicRoughnessTexFileName))
	{
		auto _wpTex = ResourceManager::Instance().GetTexture(a_fileDir + a_metallicRoughnessTexFileName);
		metallicRoughnessTexKey = a_fileDir + a_metallicRoughnessTexFileName;
		metallic = 1.0f;
		roughness = 1.0f;
	}
	// エミッシブテクスチャ
	if (!a_emissiveTexFileName.empty() && FileUtility::IsExistFile(a_fileDir + a_emissiveTexFileName))
	{
		auto _wpTex = ResourceManager::Instance().GetTexture(a_fileDir + a_emissiveTexFileName);
		emissiveTexKey = a_fileDir + a_emissiveTexFileName;
	}
	// 法線マップテクスチャ
	if (!a_normalTexFileName.empty() && FileUtility::IsExistFile(a_fileDir + a_normalTexFileName))
	{
		auto _wpTex = ResourceManager::Instance().GetTexture(a_fileDir + a_normalTexFileName);
		normalTexKey = a_fileDir + a_normalTexFileName;
	}
}
