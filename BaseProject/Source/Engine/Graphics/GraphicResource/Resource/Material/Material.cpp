#include "Material.h"

#include "Engine/Graphics/GraphicResource/Resource/Texture/Texture.h"

#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/D3D12//D3DObject/DescriptorHeap/DescriptorHeap.h"
#include "Engine/Graphics/GraphicResource/GraphicResourceManager/GraphicResourceManager.h"

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
		baseTexID = GraphicResourceManager::Instance().GetTexture(a_fileDir , a_baseColorTexFileName,TextureUse::Albedo);
		_defaultTexResource = GraphicResourceManager::Instance().NGetTexture(baseTexID)->cpResource.Get();
	}
	// メタリック・ラフネステクスチャ
	{
		if(GraphicResourceManager::Instance().GetTexture(
			metallicRoughnessTexID,
			a_fileDir , a_metallicRoughnessTexFileName, TextureUse::MetallicRoughness
		))
		{
			metallic = 1.0f;
			roughness = 1.0f;
		}
		_metallicRoughnessTexResource = GraphicResourceManager::Instance().NGetTexture(metallicRoughnessTexID)->cpResource.Get();
	}
	// エミッシブテクスチャ
	{
		emissiveTexID = GraphicResourceManager::Instance().GetTexture(a_fileDir , a_emissiveTexFileName, TextureUse::Emissive);
		_emissiveTexResource = GraphicResourceManager::Instance().NGetTexture(emissiveTexID)->cpResource.Get();
	}
	// 法線マップテクスチャ
	{
		normalTexID = GraphicResourceManager::Instance().GetTexture(a_fileDir , a_normalTexFileName, TextureUse::Normal);
		_normalTexResource = GraphicResourceManager::Instance().NGetTexture(normalTexID)->cpResource.Get();
	}

	std::vector<SRVViewInit> _resources;
	ImGuiContex::Instance().AddLog("===============================================\n");
	ImGuiContex::Instance().AddLog("Main: %p\n", _defaultTexResource);
	ImGuiContex::Instance().AddLog("MR: %p\n", _metallicRoughnessTexResource);
	ImGuiContex::Instance().AddLog("Emi: %p\n", _emissiveTexResource);
	ImGuiContex::Instance().AddLog("Nor: %p\n", _normalTexResource);
	_resources.push_back({ _defaultTexResource, nullptr });
	_resources.push_back({_metallicRoughnessTexResource, nullptr});
	_resources.push_back({_emissiveTexResource, nullptr });
	_resources.push_back({_normalTexResource, nullptr });
	
	srvHandle = DescriptorHeapManager::Instance().AllocateSRVRange(_resources);
}
