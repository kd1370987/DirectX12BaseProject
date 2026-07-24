#include "MegaBuffer.h"

#include "../../D3D12Wrapper/D3D12Wrapper.h"

namespace Engine::D3D12
{
	bool Engine::D3D12::MegaBuffer::Create(
		D3D12::Device* a_pDevice,
		D3D12::GraphicsCommandList* a_pCmdList,
		size_t a_elemetNum,
		size_t a_strideSize
	)
	{
		// リソースの作成
		GPUBufferDesc _gpuDesc = {};
		_gpuDesc.elementNum = a_elemetNum;
		_gpuDesc.strideSize = a_strideSize;
		_gpuDesc.flags = D3D12_RESOURCE_FLAG_NONE;
		_gpuDesc.heapType = D3D12_HEAP_TYPE_DEFAULT;
		if (!GPUBuffer::Create(a_pDevice, _gpuDesc))
		{
			ENGINE_ERRLOG(false, "メガバッファの作成に失敗");
			return false;
		}

		// 仕様書作成
		D3D12_SHADER_RESOURCE_VIEW_DESC _desc = {};
		_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		_desc.Format = DXGI_FORMAT_UNKNOWN;
		_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		_desc.Buffer.FirstElement = 0;
		_desc.Buffer.NumElements = static_cast<UINT>(m_elementNum);
		_desc.Buffer.StructureByteStride = static_cast<UINT>(m_strideSize);
		_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		// ハンドルをもらう
		m_srvHandle = AllocateSRV(a_pDevice, GetResource(), _desc);
	}


	void MegaBuffer::UploadDataAsync(UINT a_destOffsetBytes, const void* a_pData, UINT a_sizeBytes)
	{
		// 一時的なUploadバッファを作ってCPUデータを書き込む
		Microsoft::WRL::ComPtr<ID3D12Resource> _cpLoadBuffer;
		D3D12_HEAP_PROPERTIES _heapProps = {};
		_heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_RESOURCE_DESC _resDesc = {};
		_resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		_resDesc.Width = a_sizeBytes;
		_resDesc.Height = 1;
		_resDesc.DepthOrArraySize = 1;
		_resDesc.MipLevels = 1;
		_resDesc.Format = DXGI_FORMAT_UNKNOWN;
		_resDesc.SampleDesc.Count = 1;
		_resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		_resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		D3D12Wrapper::Instance().GetDevice()->CreateCommittedResource(
			&_heapProps,
			D3D12_HEAP_FLAG_NONE,
			&_resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&_cpLoadBuffer)
		);
		if (_cpLoadBuffer) _cpLoadBuffer->SetName(L"Mega_UploadBuffer");	// リーク調査用

		// データの書き込み
		void* _pMapped = nullptr;
		_cpLoadBuffer->Map(0, nullptr, &_pMapped);
		std::memcpy(_pMapped, a_pData, a_sizeBytes);
		_cpLoadBuffer->Unmap(0, nullptr);

		// 非同期処理に投げる
		D3D12Wrapper::Instance().ExecuteAsyncCopy(
			[this, a_destOffsetBytes, _cpLoadBuffer, a_sizeBytes](D3D12::GraphicsCommandList* a_pCmdList)
			{
				a_pCmdList->CopyBufferRegion(
					m_cpResource.Get(), a_destOffsetBytes,
					_cpLoadBuffer.Get(), 0, a_sizeBytes
				);
			},
			[_cpLoadBuffer]()
			{
				ENGINE_LOG("メガバッファの非同期アップロード完了 : メモリ解放");
			}
		);
	}

	uint64_t MegaBuffer::GetCurrentFenceValue() const
	{
		return D3D12Wrapper::Instance().GetCurrentFenceValue();
	}

	uint64_t MegaBuffer::GetNextFenceValue() const
	{
		return D3D12Wrapper::Instance().GetNextFenceValue();
	}

}
