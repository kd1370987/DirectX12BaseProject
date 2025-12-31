#pragma once

#include "../DescriptorHeap.h"

class CBV_SRV_UAVHeap : public DescriptorHeap
{
public:

	DescriptorHandle Register(ID3D12Resource* a_resource = nullptr) override;

	DescriptorHandle RegisterCBV(
		ID3D12Resource* a_resource,
		size_t a_size,
		D3D12_CONSTANT_BUFFER_VIEW_DESC& a_cbvDesc
	);

	template<typename T>
	DescriptorHandle RegisterCBV(ID3D12Resource* a_resource)
	{
		size_t _count = m_currentIndex;
		if (m_maxSize <= _count)
		{
			assert(0 && "CBVHeapのヒープ領域を使い切りました");
			return {};
		}

		// ハンドルの作成
		DescriptorHandle _handle = {};
		auto _handleCPU = m_cpHeap->GetCPUDescriptorHandleForHeapStart();
		_handleCPU.ptr += m_incrementSize * _count;
		auto _handleGPU = m_cpHeap->GetGPUDescriptorHandleForHeapStart();
		_handleGPU.ptr += m_incrementSize * _count;

		// ハンドルの登録
		_handle.handleCPU = _handleCPU;
		_handle.handleGPU = _handleGPU;

		// CBVの仕様書作成
		auto _resource = a_resource;

		D3D12_CONSTANT_BUFFER_VIEW_DESC _cbvDesc = {};
		_cbvDesc.BufferLocation = a_resource->GetGPUVirtualAddress();
		_cbvDesc.SizeInBytes = (sizeof(T) + 255) & ~255;


		// CBVの生成
		m_pDevice->CreateConstantBufferView(
			&_cbvDesc,
			_handle.handleCPU
		);

		++m_currentIndex;
		return _handle;
	}

	DescriptorHandle RegisterSRV(ID3D12Resource* a_resource = nullptr);
	DescriptorHandle RegisterUAV(ID3D12Resource* a_resource = nullptr);

private:

	// CBV・SRV・UAVのカウント
	UINT m_cbvCount = 0;
	UINT m_srvCount = 0;
	UINT m_uavCount = 0;

};