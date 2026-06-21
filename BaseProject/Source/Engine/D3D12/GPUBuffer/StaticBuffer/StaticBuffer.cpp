#include "StaticBuffer.h"

#include "../../DescriptorHeapManager/DescriptorHeapManager.h"

namespace Engine::D3D12
{
	void StaticBuffer::Release()
	{
		GPUResource::Release();
		m_gpuBuffer.Release();

		D3D12::DescriptorHeapManager::Instance().Free(m_srvHandle);
	}
	bool StaticBuffer::Create(
		ID3D12Device* a_pDevice, 
		GraphicsCommandList* a_pCmdList,
		const StaticBufferDesc& a_desc,
		const void* a_pInitData
	)
	{
		// 内容を保持・操作する自分自身であるバッファ
		DynamicBufferDesc _desc = {};
		_desc.elementNum = a_desc.elementNum;
		_desc.strideSize = a_desc.strideSize;
		_desc.flags = D3D12_RESOURCE_FLAG_NONE;
		if (!DynamicBuffer::Create(a_pDevice, _desc))
		{
			assert(0 && "リソース作成失敗");
			return false;
		}

		// 内容を初期化
		if(a_pInitData)
		{
			UpdateData(a_pInitData, GetBufferSize());
		}

		// GPUに送信するためのアップロードバッファを作成
		GPUBufferDesc _gpuDesc = {};
		_gpuDesc.elementNum = a_desc.elementNum;
		_gpuDesc.strideSize = a_desc.strideSize;
		_gpuDesc.flags = D3D12_RESOURCE_FLAG_NONE;
		_gpuDesc.heapType = D3D12_HEAP_TYPE_DEFAULT;
		if (!m_gpuBuffer.Create(a_pDevice, _gpuDesc))
		{
			assert(0 && "GPUバッファ作成失敗");
			return false;
		}

		// GPUにコピーする
		CopyToGPU(a_pCmdList);


		return true;
	}
	void StaticBuffer::Update(GraphicsCommandList* a_pCmdList)
	{
		// 更新がなければリターン
		if (!m_isDrty) return;

		// GPUにコピー
		CopyToGPU(a_pCmdList);
	}
	void StaticBuffer::UpdateData(const void* a_data, size_t a_size)
	{
		DynamicBuffer::UpdateData(a_data,a_size);
		m_isDrty = true;
	}

	void StaticBuffer::Barrier(ID3D12GraphicsCommandList* a_pCmdList, D3D12_RESOURCE_STATES a_nextState)
	{
		m_gpuBuffer.Barrier(a_pCmdList, a_nextState);
	}

	ID3D12Resource* StaticBuffer::GetResource() const
	{
		return m_gpuBuffer.GetResource();
	}

	D3D12_GPU_VIRTUAL_ADDRESS StaticBuffer::GetGPUVirtualAddress() const
	{
		return m_gpuBuffer.GetGPUVirtualAddress();
	}

	void StaticBuffer::CreateSRVInternal(ID3D12Device* a_pDevice)
	{
		// 仕様書作成
		D3D12_SHADER_RESOURCE_VIEW_DESC _desc = {};
		_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		_desc.Format = DXGI_FORMAT_UNKNOWN;
		_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		_desc.Buffer.FirstElement = 0;
		_desc.Buffer.NumElements = m_elementNum;
		_desc.Buffer.StructureByteStride = m_strideSize;
		_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		// ハンドルをもらう
		m_srvHandle = DescriptorHeapManager::Instance().Allocate<SRV>(a_pDevice, m_gpuBuffer.GetResource(), &_desc);
	}

	void StaticBuffer::CopyToGPU(GraphicsCommandList* a_pCmdList)
	{
		// コピー用にGPUバッファを変更
		m_gpuBuffer.Barrier(a_pCmdList, D3D12_RESOURCE_STATE_COPY_DEST);

		// GPUバッファにコピー
		a_pCmdList->CopyBufferRegion(
			m_gpuBuffer.GetResource(),
			0,
			m_cpResource.Get(),
			0,
			GetBufferSize()
		);

		// SRVに戻す
		m_gpuBuffer.Barrier(
			a_pCmdList,
			//D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			D3D12_RESOURCE_STATE_COMMON
		);
		m_isDrty = false;
	}
}