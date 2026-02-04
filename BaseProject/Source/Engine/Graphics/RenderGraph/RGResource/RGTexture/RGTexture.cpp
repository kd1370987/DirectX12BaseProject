#include "RGTexture.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"

#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

bool RGTexture::Create(const RGTextureDesc& a_desc)
{
	D3D12_RESOURCE_DESC _desc = {};
	_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	_desc.Width = a_desc.width;
	_desc.Height = a_desc.height;
	_desc.DepthOrArraySize = 1;
	_desc.MipLevels = static_cast<UINT16>(a_desc.mipLevel);
	_desc.Format = a_desc.format;
	_desc.SampleDesc.Count = a_desc.sampleCount;
	_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	if (a_desc.allowRTV)
	{
		_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	}
	if (a_desc.allowDSV)
	{
		_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	}
	if (a_desc.allowUAV)
	{
		_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}
	if (!a_desc.allowSRV)
	{
		
		_desc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
	}

	auto _heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	D3D12_CLEAR_VALUE _clear{};
	const D3D12_CLEAR_VALUE* _pClear = nullptr;

	if (a_desc.allowDSV)
	{
		_clear.Format = DXGI_FORMAT_D32_FLOAT;
		_clear.DepthStencil.Depth = static_cast<FLOAT>(1.0f);
		_clear.DepthStencil.Stencil = static_cast<UINT8>(0.0f);
		_pClear = &_clear;
	}
	else if (a_desc.allowRTV)
	{
	
		_clear.Format = _desc.Format;
		_clear.Color[0] = 0;
		_clear.Color[1] = 0;
		_clear.Color[2] = 0;
		_clear.Color[3] = 1;
		_pClear = &_clear;
	}
	else if (a_desc.clearValue.has_value())
	{
		_clear = a_desc.clearValue.value();
		_pClear = &_clear;
	}

	if (a_desc.allowDSV)
	{
		HRESULT _hr = D3D12Wrapper::Instance().GetDevice()->CreateCommittedResource(
			&_heapProp,
			D3D12_HEAP_FLAG_NONE,
			&_desc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			_pClear,
			IID_PPV_ARGS(&m_cpResource)
		);
		if (FAILED(_hr))
		{
			assert(0 && "RGTextureの生成に失敗");
			return false;
		}
	}
	else
	{
		HRESULT _hr = D3D12Wrapper::Instance().GetDevice()->CreateCommittedResource(
			&_heapProp,
			D3D12_HEAP_FLAG_NONE,
			&_desc,
			D3D12_RESOURCE_STATE_COMMON,
			_pClear,
			IID_PPV_ARGS(&m_cpResource)
		);
		if (FAILED(_hr))
		{
			assert(0 && "RGTextureの生成に失敗");
			return false;
		}
	}



	if (a_desc.allowRTV)
	{
		D3D12_RENDER_TARGET_VIEW_DESC _rtvDesc = {};
		_rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		_rtvDesc.Format = a_desc.format;
		m_rtvHandle = DescriptorHeapManager::Instance().RegisterRTV(m_cpResource.Get(),&_rtvDesc);
	}
	if (a_desc.allowDSV)
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC _dsv{};
		_dsv.Format = DXGI_FORMAT_D32_FLOAT;
		_dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		m_dsvHandle = 
			DescriptorHeapManager::Instance().RefDSVHeap().RegisterDSV(m_cpResource.Get(),&_dsv);
	}
	if (a_desc.allowUAV)
	{
		
	}
	if (a_desc.allowSRV)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
		if (a_desc.allowDSV)
		{
			srv.Format = DXGI_FORMAT_R32_FLOAT;
		}
		else
		{
			srv.Format = a_desc.format;
		}
		srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv.Texture2D.MipLevels = a_desc.mipLevel;
		srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		SRVViewInit _srvInit = {};
		_srvInit.pDesc = &srv;
		_srvInit.pResource = m_cpResource.Get();
		m_srvHandle = DescriptorHeapManager::Instance().AllocateSRVRange({_srvInit});
	}

	m_desc = a_desc;

	return true;
}

bool RGTexture::Create(const D3D12_RESOURCE_DESC& a_desc)
{
	auto _desc = a_desc;

	// ヒーププロパティの作成
	D3D12_HEAP_PROPERTIES _heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	// クリアバリューの作成
	float _clsClr[4] = { 0.0f,0.0f,0.0f,1.0f };
	D3D12_CLEAR_VALUE _clearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, _clsClr);

	HRESULT _hr = D3D12Wrapper::Instance().GetDevice()->CreateCommittedResource(
		&_heapProp,
		D3D12_HEAP_FLAG_NONE,
		&a_desc,
		D3D12_RESOURCE_STATE_COMMON,
		&_clearValue,
		IID_PPV_ARGS(&m_cpResource)
	);
	if (FAILED(_hr))
	{
		assert(0 && "RGTextureの生成に失敗");
		return false;
	}

	
	D3D12_RENDER_TARGET_VIEW_DESC _rtvDesc = {};
	_rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	_rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	m_rtvHandle = DescriptorHeapManager::Instance().RegisterRTV(m_cpResource.Get(), &_rtvDesc);


	D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
	srv.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv.Texture2D.MipLevels = 1;
	srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	SRVViewInit _srvInit = {};
	_srvInit.pDesc = &srv;
	_srvInit.pResource = m_cpResource.Get();
	m_srvHandle = DescriptorHeapManager::Instance().AllocateSRVRange({ _srvInit });
	
	return true;
}

D3D12_CPU_DESCRIPTOR_HANDLE RGTexture::GetSRVHandle()
{
	return DescriptorHeapManager::Instance().GetSRVCPUHandle(m_srvHandle);
}

D3D12_GPU_DESCRIPTOR_HANDLE RGTexture::GPUSRVHandle()
{
	return DescriptorHeapManager::Instance().GetSRVGPUHandle(m_srvHandle);
}

D3D12_CPU_DESCRIPTOR_HANDLE RGTexture::GetRTVHandle()
{
	return DescriptorHeapManager::Instance().GetRTVCPUHandle(m_rtvHandle);
}

D3D12_CPU_DESCRIPTOR_HANDLE RGTexture::GetDSVHandle()
{
	return DescriptorHeapManager::Instance().RefDSVHeap().GetCPUHandle(m_dsvHandle);
}

