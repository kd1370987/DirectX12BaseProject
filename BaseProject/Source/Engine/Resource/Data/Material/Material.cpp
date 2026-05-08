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
	baseColorTex = Engine::Resource::TextureLoader::Request(a_fileDir + a_baseColorTexFileName,TexColor::WHITE);
	metaRoughTex = Engine::Resource::TextureLoader::Request(a_fileDir + a_metallicRoughnessTexFileName, { 0,255,255,255 });
	emissiveTex = Engine::Resource::TextureLoader::Request(a_fileDir + a_emissiveTexFileName, { 0,0,0,255 });
	normalTex = Engine::Resource::TextureLoader::Request(a_fileDir + a_normalTexFileName, { 128,128,255,255 });
}
