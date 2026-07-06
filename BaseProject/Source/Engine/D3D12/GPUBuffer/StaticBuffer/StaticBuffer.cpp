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
		D3D12::Device* a_pDevice, 
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

	void StaticBuffer::UploadDataRange(D3D12::GraphicsCommandList* a_pCmdList, size_t a_destOffsetBytes, const void* a_pData, size_t a_sizeBytes)
	{
		if (!a_pData || a_sizeBytes == 0) return;

		// CPU側のアップロードバッファの特定領域のみを更新する
		this->UpdateDataOffset(a_pData, a_sizeBytes, a_destOffsetBytes);

		//// GPUバッファをコピー先に遷移
		//m_gpuBuffer.Barrier(a_pCmdList, D3D12_RESOURCE_STATE_COPY_DEST);

		// 部分コピーコマンドを積む
		a_pCmdList->CopyBufferRegion(
			m_gpuBuffer.GetResource(),
			a_destOffsetBytes,       // コピー先のオフセット
			m_cpResource.Get(),      // コピー元（Uploadバッファ）
			a_destOffsetBytes,       // コピー元のオフセット（通常はコピー先と同じ場所を使います）
			a_sizeBytes              // コピーするサイズ
		);

		//// SRVとして読める状態に戻す
		//m_gpuBuffer.Barrier(a_pCmdList, D3D12_RESOURCE_STATE_COMMON);

		m_isDrty = true;
	}

	void StaticBuffer::UploadDataRange(D3D12::GraphicsCommandList* a_pCmdList, UINT a_startIndex, UINT a_count, const void* a_pData)
	{
		UploadDataRange(
			a_pCmdList,
			a_startIndex * m_strideSize,
			a_pData,
			a_count * m_strideSize
		);
	}

	void StaticBuffer::Barrier(D3D12::GraphicsCommandList* a_pCmdList, D3D12_RESOURCE_STATES a_nextState)
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

	void StaticBuffer::CreateSRVInternal(D3D12::Device* a_pDevice)
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