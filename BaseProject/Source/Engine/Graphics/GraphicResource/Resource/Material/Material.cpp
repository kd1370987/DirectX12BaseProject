#include "Material.h"

#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/D3D12//D3DObject/DescriptorHeap/DescriptorHeap.h"
#include "Engine/Graphics/GraphicResource/GraphicResourceManager/GraphicResourceManager.h"

#include "Engine/Resource/Manager/TextureManager/TextureManager.h"

void Engine::Resource::Material::SetTexture2D(
	const std::string& a_fileDir,
	const std::string& a_baseColorTexFileName,
	const std::string& a_metallicRoughnessTexFileName,
	const std::string& a_emissiveTexFileName,
	const std::string& a_normalTexFileName
)
{
	std::vector<TextureInit> _initVec = {};
	_initVec.push_back({ a_fileDir + a_baseColorTexFileName ,{255,255,255,255} });
	if (a_metallicRoughnessTexFileName == "")
	{
		_initVec.push_back({ a_fileDir + "metaRough",{0,255,255,255} });
	}
	else
	{
		_initVec.push_back({ a_fileDir + a_metallicRoughnessTexFileName,{0,255,255,255} });
	}
	if (a_emissiveTexFileName == "")
	{
		_initVec.push_back({ a_fileDir + "emissive",{0,0,0,255} });
	}
	else
	{
		_initVec.push_back({ a_fileDir + a_emissiveTexFileName ,{0,0,0,255} });
	}
	if (a_normalTexFileName == "")
	{
		_initVec.push_back({ a_fileDir + "normal",{128,128,255,255} });
	}
	else
	{
		_initVec.push_back({ a_fileDir + a_normalTexFileName,{128,128,255,255} });
	}
	

	auto _handles = Engine::Resource::TextureManager::Instance().LoadTextureRange(_initVec);
	baseColorTex = _handles[0];
	metaRoughTex = _handles[1];
	emissiveTex	 = _handles[2];
	normalTex	 = _handles[3];

	auto& _startTex = Engine::Resource::TextureManager::Instance().GetTexture(baseColorTex);
	startSRVHandle = _startTex.GetSRV();
}
