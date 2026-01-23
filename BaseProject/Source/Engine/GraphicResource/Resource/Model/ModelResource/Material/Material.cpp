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
	{
		auto _wpTex = GraphicResourceManager::Instance().GetTexture(a_fileDir + a_baseColorTexFileName);
		baseTexID = _wpTex;
		_defaultTexResource = GraphicResourceManager::Instance().NGetTexture(_wpTex)->GetResource();
	}
	// メタリック・ラフネステクスチャ
	{
		auto _wpTex = GraphicResourceManager::Instance().GetTexture(a_fileDir + a_metallicRoughnessTexFileName);
		metallicRoughnessTexID = _wpTex;
		metallic = 1.0f;
		roughness = 1.0f;
		_metallicRoughnessTexResource = GraphicResourceManager::Instance().NGetTexture(_wpTex)->GetResource();
	}
	// エミッシブテクスチャ
	{
		auto _wpTex = GraphicResourceManager::Instance().GetTexture(a_fileDir + a_emissiveTexFileName);
		emissiveTexID = _wpTex;
		_emissiveTexResource = GraphicResourceManager::Instance().NGetTexture(_wpTex)->GetResource();
	}
	// 法線マップテクスチャ
	{
		auto _wpTex = GraphicResourceManager::Instance().GetTexture(a_fileDir + a_normalTexFileName);
		normalTexID = _wpTex;
		_normalTexResource = GraphicResourceManager::Instance().NGetTexture(_wpTex)->GetResource();
	}

	
	std::vector<ID3D12Resource*> _resources;
	_resources.push_back(_defaultTexResource);
	_resources.push_back(_metallicRoughnessTexResource);
	_resources.push_back(_emissiveTexResource);
	_resources.push_back(_normalTexResource);
	
	srvHandle = DescriptorHeapManager::Instance().AllocateSRVRange(_resources);
}
