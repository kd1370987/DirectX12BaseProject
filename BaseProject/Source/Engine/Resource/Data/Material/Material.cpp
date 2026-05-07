#include "Material.h"

#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/D3D12//D3DObject/DescriptorHeap/DescriptorHeap.h"

//#include "Engine/Resource/Manager/TextureManager/TextureManager.h"
#include "Engine/Resource/Loader/Texture/TextureLoader.h"

void Engine::Resource::Material::SetTexture2D(
	const std::string& a_fileDir,
	const std::string& a_baseColorTexFileName,
	const std::string& a_metallicRoughnessTexFileName,
	const std::string& a_emissiveTexFileName,
	const std::string& a_normalTexFileName
)
{
	//baseColorTex = Engine::Resource::TextureManager::Instance().LoadTexture(a_fileDir + a_baseColorTexFileName, { 255,255,255,255 });
	baseColorTex = Engine::Resource::TextureLoader::Request(a_fileDir + a_baseColorTexFileName,TexColor::WHITE);
	//if (a_metallicRoughnessTexFileName == "")
	//{
	//	metaRoughTex = Engine::Resource::TextureManager::Instance().LoadTexture(a_fileDir + "metaRough",{0,255,255,255});
	//}
	//else
	//{
	//	metaRoughTex = Engine::Resource::TextureManager::Instance().LoadTexture(a_fileDir + a_metallicRoughnessTexFileName, { 0,255,255,255 });
	//}
	metaRoughTex = Engine::Resource::TextureLoader::Request(a_fileDir + a_metallicRoughnessTexFileName, { 0,255,255,255 });
	//if (a_emissiveTexFileName == "")
	//{
	//	emissiveTex = Engine::Resource::TextureManager::Instance().LoadTexture(a_fileDir + "emissive", { 0,0,0,255 });
	//}
	//else
	//{
	//	emissiveTex = Engine::Resource::TextureManager::Instance().LoadTexture(a_fileDir + a_emissiveTexFileName, { 0,0,0,255 });
	//}
	emissiveTex = Engine::Resource::TextureLoader::Request(a_fileDir + "emissive", { 0,0,0,255 });
	//if (a_normalTexFileName == "")
	//{
	//	normalTex = Engine::Resource::TextureManager::Instance().LoadTexture(a_fileDir + "normal", { 128,128,255,255 });
	//}
	//else
	//{
	//	normalTex = Engine::Resource::TextureManager::Instance().LoadTexture(a_fileDir + a_normalTexFileName, { 128,128,255,255 });
	//}
	normalTex = Engine::Resource::TextureLoader::Request(a_fileDir + a_normalTexFileName, { 128,128,255,255 });
}
