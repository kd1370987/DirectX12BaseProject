#include "Texture.h"

#include "../../Importer/Texture/TextureImporter.h"
#include "../../Create/Texture/TextureCreater.h"

#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

void Engine::Resource::TextureRes::Import(
	const std::string& a_filePath,
	const DirectX::XMFLOAT4& a_defoltData
)
{
	// テクスチャの読み込み
	auto _texDesc = Engine::Resource::ImportTexture(a_filePath);
	if(_texDesc.cpResource)
	{
		// 読み込み成功時
		m_cpResource = _texDesc.cpResource;
		m_desc = _texDesc.desc;
		m_name = FileUtility::GetFileName(a_filePath);
	}
	else
	{
		// 読み込み失敗時はデフォルトデータを指定してテクスチャを生成する
		_texDesc = Engine::Resource::DefaultTexture(a_defoltData);
		m_cpResource = _texDesc.cpResource;
		m_desc = _texDesc.desc;
		m_name = "DefaultTexture";
	}

	// SRVとして使用
	m_useFlg = TextureUsage::SRV;
}

void Engine::Resource::TextureRes::Create(
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

	if (HasFlag(a_usage, TextureUsage::RTV))
	{
		// レンダーターゲット作成
		D3D12_RENDER_TARGET_VIEW_DESC _rtvDesc = {};
		_rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		_rtvDesc.Format = m_desc.Format;
		m_rtvHandle = DescriptorHeapManager::Instance().RegisterRTV(m_cpResource.Get(),&_rtvDesc);
	}
	if (HasFlag(a_usage, TextureUsage::DSV))
	{
		// 深度ステンシル作成
		D3D12_DEPTH_STENCIL_VIEW_DESC _dsvDesc = {};
		_dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		_dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		m_dsvHandle = DescriptorHeapManager::Instance().RefDSVHeap().RegisterDSV(m_cpResource.Get(),&_dsvDesc);
	}
	if (HasFlag(a_usage, TextureUsage::UAV))
	{
	}
	if (HasFlag(a_usage, TextureUsage::SRV))
	{
		// シェーダーリソースビュー作成
		D3D12_SHADER_RESOURCE_VIEW_DESC _srvDesc = {};
		_srvDesc.Format = HasFlag(a_usage, TextureUsage::DSV) ? DXGI_FORMAT_R32_FLOAT : m_desc.Format;
		_srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		_srvDesc.Texture2D.MipLevels = m_desc.MipLevels;
		_srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		SRVViewInit _initData = {};
		_initData.pResource = m_cpResource.Get();
		_initData.pDesc = &_srvDesc;
		m_srvHandle = DescriptorHeapManager::Instance().AllocateSRVRange({ _initData });
		m_imguiSRVHandle = DescriptorHeapManager::Instance().AllocateImGuiSRVRange({ _initData });
	}
}

void Engine::Resource::TextureRes::SetName(const std::string& a_name)
{
	m_name = a_name;
}

ID3D12Resource* Engine::Resource::TextureRes::GetResource()
{
	return m_cpResource.Get();
}
