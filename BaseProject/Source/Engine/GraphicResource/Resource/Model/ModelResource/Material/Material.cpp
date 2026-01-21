#include "Material.h"

#include "Engine/GraphicResource/Resource/Texture/Texture.h"

#include "Engine/Graphics/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/D3D12//D3DObject/DescriptorHeap/DescriptorHeap.h"
#include "Engine/GraphicResource/GraphicResourceManager/GraphicResourceManager.h"

void Material::SetTexture2D(
	const std::string& a_fileDir, 
	const std::string& a_baseColorTexFileName, 
	const std::string& a_metallicRoughnessTexFileName, 
	const std::string& a_emissiveTexFileName,
	const std::string& a_normalTexFileName
)
{
	ID3D12Resource* _defaultTexResource = nullptr;
	ID3D12Resource* _metallicRoughnessTexResource = nullptr;
	ID3D12Resource* _emissiveTexResource = nullptr;
	ID3D12Resource* _normalTexResource = nullptr;
	// 基本色テクスチャ
	//if (!a_baseColorTexFileName.empty() && FileUtility::IsExistFile(a_fileDir + a_baseColorTexFileName))
	{
		auto _wpTex = GraphicResourceManager::Instance().GetTexture(a_fileDir + a_baseColorTexFileName);
		baseColorTexKey = a_fileDir + a_baseColorTexFileName;
		_defaultTexResource = _wpTex.lock()->GetResource();
	}
	// メタリック・ラフネステクスチャ
	//if (!a_metallicRoughnessTexFileName.empty() && FileUtility::IsExistFile(a_fileDir + a_metallicRoughnessTexFileName))
	{
		auto _wpTex = GraphicResourceManager::Instance().GetTexture(a_fileDir + a_metallicRoughnessTexFileName);
		metallicRoughnessTexKey = a_fileDir + a_metallicRoughnessTexFileName;
		metallic = 1.0f;
		roughness = 1.0f;
		_metallicRoughnessTexResource = _wpTex.lock()->GetResource();
	}
	// エミッシブテクスチャ
	///if (!a_emissiveTexFileName.empty() && FileUtility::IsExistFile(a_fileDir + a_emissiveTexFileName))
	{
		auto _wpTex = GraphicResourceManager::Instance().GetTexture(a_fileDir + a_emissiveTexFileName);
		emissiveTexKey = a_fileDir + a_emissiveTexFileName;
		_emissiveTexResource = _wpTex.lock()->GetResource();
	}
	// 法線マップテクスチャ
//	if (!a_normalTexFileName.empty() && FileUtility::IsExistFile(a_fileDir + a_normalTexFileName))
	{
		auto _wpTex = GraphicResourceManager::Instance().GetTexture(a_fileDir + a_normalTexFileName);
		normalTexKey = a_fileDir + a_normalTexFileName;
		_normalTexResource = _wpTex.lock()->GetResource();
	}

	
	std::vector<ID3D12Resource*> _resources;
	_resources.push_back(_defaultTexResource);
	_resources.push_back(_metallicRoughnessTexResource);
	_resources.push_back(_emissiveTexResource);
	_resources.push_back(_normalTexResource);
	
	srvHandle = DescriptorHeapManager::Instance().AllocateSRVRange(_resources);
}
