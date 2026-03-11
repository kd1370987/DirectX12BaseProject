#include "Texture.h"

#include "../../Importer/Texture/TextureImporter.h"
#include "../../Create/Texture/TextureCreater.h"

#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

void Engine::Resource::Texture::Import(
	const std::string& a_filePath,
	const DirectX::XMFLOAT4& a_defoltData
)
{
	// テクスチャの読み込み
	ComPtr<ID3D12Resource> _cpRes = Engine::Resource::ImportTexture(a_filePath);
	if(_cpRes)
	{
		// 読み込み成功時
		m_cpResource = _cpRes;
		m_desc = _cpRes.Get()->GetDesc();
		m_name = a_filePath;
	}
	else
	{
		// 読み込み失敗時はデフォルトデータを指定してテクスチャを生成する
		_cpRes = Engine::Resource::DefaultTexture(a_defoltData);
		m_cpResource = _cpRes;
		m_desc = _cpRes.Get()->GetDesc();
		m_name = a_filePath + "Default";
	}

	// SRVとして使用
	m_useFlg = TextureUsage::SRV;
}

void Engine::Resource::Texture::Create(
	const UINT64& a_width,
	const UINT& a_height, 
	const DXGI_FORMAT& a_format,
	const TextureUsage& a_usage
)
{
	// 仕様書作成
	TextureCreateDesc _desc = {};
	_desc.width = a_width;
	_desc.height = a_height;
	_desc.usage = a_usage;
	_desc.format = a_format;

	// リソース作成
	m_cpResource = Engine::Resource::CreateTexture(_desc, &m_desc);

	// 名前決定
	m_name = "CreateTextures";

	m_useFlg = a_usage;
}

void Engine::Resource::Texture::SetName(const std::string& a_name)
{
	m_name = a_name;
}

const std::string& Engine::Resource::Texture::GetName()
{
	return m_name;
}

ID3D12Resource* Engine::Resource::Texture::GetResource()
{
	return m_cpResource.Get();
}

const Engine::Resource::TextureUsage& Engine::Resource::Texture::GetUsage() const
{
	return m_useFlg;
}

const D3D12_RESOURCE_DESC& Engine::Resource::Texture::GetDesc() const
{
	return m_desc;
}

const Engine::Resource::Handle<RTV>& Engine::Resource::Texture::GetRTV() const
{
	return m_rtvHandle;
}

const Engine::Resource::Handle<DSV>& Engine::Resource::Texture::GetDSV() const
{
	return m_dsvHandle;
}

const Engine::Resource::Handle<SRV>& Engine::Resource::Texture::GetSRV() const
{
	return m_srvHandle;
}

const Engine::Resource::Handle<UAV>& Engine::Resource::Texture::GetUAV() const
{
	return m_uavHandle;
}

const Engine::Resource::Handle<SRV>& Engine::Resource::Texture::GetImGuiSRV() const
{
	return m_imguiSRVHandle;
}

void Engine::Resource::Texture::SetRTV(const Engine::Resource::Handle<RTV>& a_handle)
{
	m_rtvHandle = a_handle;
}

void Engine::Resource::Texture::SetDSV(const Engine::Resource::Handle<DSV>& a_handle)
{
	m_dsvHandle = a_handle;
}

void Engine::Resource::Texture::SetSRV(const Engine::Resource::Handle<SRV>& a_handle)
{
	m_srvHandle = a_handle;
}

void Engine::Resource::Texture::SetUAV(const Engine::Resource::Handle<UAV>& a_handle)
{
	m_uavHandle = a_handle;
}

void Engine::Resource::Texture::SetImGuiSRV(const Engine::Resource::Handle<SRV>& a_handle)
{
	m_imguiSRVHandle = a_handle;
}
