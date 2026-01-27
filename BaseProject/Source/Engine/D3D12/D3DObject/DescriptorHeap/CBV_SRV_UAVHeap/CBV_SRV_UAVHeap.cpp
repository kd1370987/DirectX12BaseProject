#include "CBV_SRV_UAVHeap.h"

#include "Engine/D3D12/D3D12Wrapper/RenderingEngine.h"

bool CBV_SRV_UAVHeap::Create(const CBV_SRV_UAVInitInfo& a_info)
{
	m_pDevice = a_info.pDevice;
	if (m_pDevice == nullptr)
	{
		assert(0 && "デバイスがnullptrです\n");
		return false;
	}

	m_srvRange.Init(a_info.maxSRVCount);


	// ディスクリプタヒープの仕様書作成
	D3D12_DESCRIPTOR_HEAP_DESC _desc = {};
	_desc.NodeMask = a_info.mask;
	_desc.Type = a_info.type;
	_desc.NumDescriptors = static_cast<UINT>(a_info.maxCBVCount + (a_info.maxSRVCount + a_info.useImGuiSRVCount) + a_info.maxUAVCount);
	_desc.Flags = a_info.flags;

	// ディスクリプタヒープの生成
	auto _hr = m_pDevice->CreateDescriptorHeap(
		&_desc,
		IID_PPV_ARGS(m_cpHeap.ReleaseAndGetAddressOf())
	);
	if (FAILED(_hr))
	{
		assert(0 && "ディスクリプタヒープ作成失敗\n");
		return false;
	}

	// インクリメントサイズの取得
	m_incrementSize = m_pDevice->GetDescriptorHandleIncrementSize(_desc.Type);
	m_type = a_info.type;
	m_initInfo = a_info;

	return true;
}

ID3D12DescriptorHeap* CBV_SRV_UAVHeap::GetHeap()
{
	return m_cpHeap.Get();
}

Storage::Range CBV_SRV_UAVHeap::AllocateSRVRange(std::vector<SRVViewInit> a_initVec)
{
	// 領域確保
	UINT _resSize = static_cast<UINT>(a_initVec.size());		// 確保予定のサイズ
	Storage::Range _range = m_srvRange.Allocate(_resSize);

	// ハンドルの作成
	auto _handle = m_cpHeap->GetCPUDescriptorHandleForHeapStart();
	_handle.ptr += m_incrementSize * (static_cast<UINT>(m_initInfo.maxCBVCount) + _range.startIndex);
		
	// SRVの仕様書作成
	for (UINT _i = 0; _i < a_initVec.size(); ++_i)
	{
		auto& _res = a_initVec[_i];

		if (_res.pResource == nullptr)
		{
			continue;
		}

		// SRVの仕様書があればそれを使う
		D3D12_SHADER_RESOURCE_VIEW_DESC _srvDesc = {};
		if (a_initVec[_i].pDesc)
		{
			_srvDesc = *a_initVec[_i].pDesc;
		}
		{
			_srvDesc.Format = _res.pResource->GetDesc().Format;
			_srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			_srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			_srvDesc.Texture2D.MipLevels = 1;
		}

		_handle.ptr += static_cast<unsigned int>(m_incrementSize * _i);

		// SRVの生成
		m_pDevice->CreateShaderResourceView(
			_res.pResource,
			&_srvDesc,
			_handle
		);
	}

	return _range;
}

D3D12_CPU_DESCRIPTOR_HANDLE CBV_SRV_UAVHeap::GetSRVCPUHandle(Storage::Range a_handle)
{
	D3D12_CPU_DESCRIPTOR_HANDLE _handleCPU = m_cpHeap->GetCPUDescriptorHandleForHeapStart();
	_handleCPU.ptr += m_incrementSize * (static_cast<UINT>(m_initInfo.maxCBVCount) + a_handle.startIndex);
	return _handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE CBV_SRV_UAVHeap::GetSRVGPUHandle(Storage::Range a_handle)
{
	D3D12_GPU_DESCRIPTOR_HANDLE _handleGPU = m_cpHeap->GetGPUDescriptorHandleForHeapStart();
	_handleGPU.ptr += m_incrementSize * (static_cast<UINT>(m_initInfo.maxCBVCount) + a_handle.startIndex);
	return _handleGPU;
}

DescriptorHandle CBV_SRV_UAVHeap::RegisterUAV(ID3D12Resource* a_resource)
{
	return DescriptorHandle();
}

D3D12_CPU_DESCRIPTOR_HANDLE CBV_SRV_UAVHeap::GetUAVCPUHandle(UAVHandle a_handle)
{
	D3D12_CPU_DESCRIPTOR_HANDLE _handleCPU = m_cpHeap->GetCPUDescriptorHandleForHeapStart();
	UINT _index = (
		static_cast<UINT>(m_initInfo.maxCBVCount) +
		static_cast<UINT>(m_initInfo.maxSRVCount) +
		static_cast<UINT>(m_initInfo.useImGuiSRVCount)
	);
	_index += a_handle.index;
	_handleCPU.ptr += m_incrementSize * _index;
	return _handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE CBV_SRV_UAVHeap::GetUAVGPUHandle(UAVHandle a_handle)
{
	D3D12_GPU_DESCRIPTOR_HANDLE _handleGPU = m_cpHeap->GetGPUDescriptorHandleForHeapStart();
	UINT _index = (
		static_cast<UINT>(m_initInfo.maxCBVCount) +
		static_cast<UINT>(m_initInfo.maxSRVCount) +
		static_cast<UINT>(m_initInfo.useImGuiSRVCount)
		);
	_index += a_handle.index;
	_handleGPU.ptr += m_incrementSize * _index;
	return _handleGPU;
}

D3D12_CPU_DESCRIPTOR_HANDLE CBV_SRV_UAVHeap::GetImGuiCPUHandle()
{
	D3D12_CPU_DESCRIPTOR_HANDLE _handleCPU = m_cpHeap->GetCPUDescriptorHandleForHeapStart();
	_handleCPU.ptr += 
		m_incrementSize * (static_cast<UINT>(m_initInfo.maxCBVCount) + static_cast<UINT>(m_initInfo.maxSRVCount));
	return _handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE CBV_SRV_UAVHeap::GetImGuiGPUHandle()
{
	D3D12_GPU_DESCRIPTOR_HANDLE _handleGPU = m_cpHeap->GetGPUDescriptorHandleForHeapStart();
	_handleGPU.ptr += 
		m_incrementSize * (static_cast<UINT>(m_initInfo.maxCBVCount) + static_cast<UINT>(m_initInfo.maxSRVCount));
	return _handleGPU;
}

