#include "Texture.h"

#include "../../Loader/Texture/Importer/TextureImporter.h"
#include "../../Loader/Texture/Creater/TextureCreater.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
namespace Engine::Resource
{
	void Engine::Resource::Texture::Import(
		const std::string& a_filePath,
		const DirectX::XMFLOAT4& a_defoltData
	)
	{
		// SRVとして使用
		m_useFlg = TextureUsage::SRV;
		// テクスチャの読み込み
		ComPtr<ID3D12Resource> _cpRes = Engine::Resource::ImportTexture(a_filePath);
		if (_cpRes)
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

		// ビューの登録
		CreateView();
	}

	void Texture::Create(const std::string& a_name, const DirectX::XMFLOAT4& a_defoltData)
	{

		ComPtr<ID3D12Resource> _cpRes = nullptr;
		_cpRes = DefaultTexture(a_defoltData);
		m_cpResource = _cpRes;
		m_desc = _cpRes.Get()->GetDesc();
		m_name = a_name;

		// SRVとして使用
		m_useFlg = TextureUsage::SRV;

		// ビューの登録
		CreateView();
	}

	void Engine::Resource::Texture::Create(const TextureCreateDesc& a_desc)
	{
		// リソース作成 : GPUリソースの作成は使っていない
		m_cpResource = Engine::Resource::CreateTexture(a_desc, &m_desc);
		m_currentState = D3D12_RESOURCE_STATE_COMMON;

		m_cpResource.Get()->SetName(StringUtility::ToWideString(a_desc.name).c_str());

		// 変数保存
		m_name = a_desc.name;
		m_useFlg = a_desc.usage;
		if (a_desc.opClerValue.has_value())
		{
			m_clearValue = a_desc.opClerValue.value();
		}
		// ビューの登録
		CreateView();
	}

	void Texture::Create(IDXGISwapChain* a_pSwapChain, UINT a_backBufferIndex, TextureUsage a_texUsage)
	{
		// スワップチェインからバックバッファを生成
		a_pSwapChain->GetBuffer(
			a_backBufferIndex,
			IID_PPV_ARGS(m_cpResource.ReleaseAndGetAddressOf())
		);
		m_currentState = D3D12_RESOURCE_STATE_PRESENT;
		// 名前設定
		std::string _name = "BackBuffer_" + a_backBufferIndex;
		m_cpResource->SetName(StringUtility::ToWideString(_name).c_str());

		// メンバ作成
		m_name = _name;
		m_useFlg = a_texUsage;
		CreateView();
	}

	void Texture::Release()
	{
		ENGINE_LOG("テクスチャの解放 : Release");
		GPUResource::Release();
	}

	void Texture::CreateView()
	{
		// デバイスの取得
		auto* _pDevice = D3D12::D3D12Wrapper::Instance().GetDevice();
		if (!_pDevice) { assert(0 && "Not fined Device"); return; }

		// テクスチャの使用方法にRTが含まれているのならRTVを登録
		if (HasFlag(m_useFlg, TextureUsage::RTV))
		{
			// レンダーターゲットビュー情報の作成
			D3D12_RENDER_TARGET_VIEW_DESC _rtvDesc = {};
			_rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			_rtvDesc.Format = m_desc.Format;

			// ディスクリプタヒープに登録
			m_rtvHandle =
				D3D12::DescriptorHeapManager::Instance().Allocate<D3D12::RTV>(_pDevice, m_cpResource.Get(), &_rtvDesc);
		}

		// テクスチャの使用方法にDSが含まれているのなら
		if (HasFlag(m_useFlg, TextureUsage::DSV))
		{
			// 深度ステンシルビュー作成
			D3D12_DEPTH_STENCIL_VIEW_DESC _dsvDesc = {};
			_dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
			_dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

			// ディスクリプタヒープに登録
			m_dsvHandle =
				D3D12::DescriptorHeapManager::Instance().Allocate<D3D12::DSV>(_pDevice, m_cpResource.Get(), &_dsvDesc);

			// リードオンリーのDSV作成
			_dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
			m_readOnlyDsvHandle =
				D3D12::DescriptorHeapManager::Instance().Allocate<D3D12::DSV>(_pDevice, m_cpResource.Get(), &_dsvDesc);
		}

		// テクスチャの使用方法にUAが含まれているのなら
		if (HasFlag(m_useFlg, TextureUsage::UAV))
		{
			m_uavHandle =
				D3D12::DescriptorHeapManager::Instance().Allocate<D3D12::UAV>(_pDevice, m_cpResource.Get(), nullptr);
		}

		// テクスチャの仕様方法SRが含まれているのなら
		if (HasFlag(m_useFlg, TextureUsage::SRV))
		{
			// シェーダーリソースビュー作成
			D3D12_SHADER_RESOURCE_VIEW_DESC _srvDesc = {};
			_srvDesc.Format = HasFlag(m_useFlg, TextureUsage::DSV) ? DXGI_FORMAT_R32_FLOAT : m_desc.Format;
			_srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			_srvDesc.Texture2D.MipLevels = m_desc.MipLevels;
			_srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

			// 登録
			m_srvHandle = 
				D3D12::DescriptorHeapManager::Instance().Allocate<D3D12::SRV>(_pDevice, m_cpResource.Get(), &_srvDesc);

			// 同時にImGuiも登録
			m_imguiSRVHandle = 
				D3D12::DescriptorHeapManager::Instance().AllocateImGuiSRV(m_cpResource.Get(), &_srvDesc);
		}
	}

	void Engine::Resource::Texture::SetName(const std::string& a_name)
	{
		m_name = a_name;
		m_cpResource.Get()->SetName(StringUtility::ToWideString(m_name).c_str());
	}

	const std::string& Engine::Resource::Texture::GetName() const
	{
		return m_name;
	}


	const Engine::Resource::TextureUsage& Engine::Resource::Texture::GetUsage() const
	{
		return m_useFlg;
	}

	const D3D12_RESOURCE_DESC& Engine::Resource::Texture::GetDesc() const
	{
		return m_desc;
	}
}