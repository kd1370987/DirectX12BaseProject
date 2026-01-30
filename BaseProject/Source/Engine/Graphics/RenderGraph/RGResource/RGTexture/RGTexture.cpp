#include "RGTexture.h"

#include "Engine/D3D12/D3D12Wrapper/RenderingEngine.h"

#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

bool RGTexture::Create(const RGTextureDesc& a_desc)
{
	D3D12_RESOURCE_DESC _desc = {};
	_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	_desc.Width = a_desc.width;
	_desc.Height = a_desc.height;
	_desc.DepthOrArraySize = 1;
	_desc.MipLevels = a_desc.mipLevel;
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

	const D3D12_CLEAR_VALUE* _pClear = nullptr;

	D3D12_CLEAR_VALUE _clear{};
	if (a_desc.clearValue.has_value() &&
		(a_desc.allowRTV || a_desc.allowDSV))
	{
		_clear = a_desc.clearValue.value();
		_pClear = &_clear;
	}

	HRESULT _hr = RenderingEngine::Instance().GetDevice()->CreateCommittedResource(
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

	if (a_desc.allowRTV)
	{
		D3D12_RENDER_TARGET_VIEW_DESC _rtvDesc = {};
		_rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		_rtvDesc.Format = a_desc.format;
		m_rtvHandle = DescriptorHeapManager::Instance().RegisterRTV(m_cpResource.Get(),&_rtvDesc);
	}
	if (a_desc.allowDSV)
	{
		
	}
	if (a_desc.allowUAV)
	{
		
	}
	if (!a_desc.allowSRV)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
		srv.Format = a_desc.format;
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

D3D12_CPU_DESCRIPTOR_HANDLE RGTexture::GetSRVHandle()
{
	return DescriptorHeapManager::Instance().GetSRVCPUHandle(m_srvHandle);
}

D3D12_CPU_DESCRIPTOR_HANDLE RGTexture::GetRTVHandle()
{
	return DescriptorHeapManager::Instance().GetRTVCPUHandle(m_rtvHandle);
}
