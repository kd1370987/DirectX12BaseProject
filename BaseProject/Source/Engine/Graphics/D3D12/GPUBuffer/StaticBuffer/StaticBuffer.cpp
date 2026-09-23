#include "StaticBuffer.h"

#include "../../DescriptorHeapManager/DescriptorHeapManager.h"

namespace Engine::D3D12
{
	void StaticBuffer::Release()
	{
		// SRVの返却は GPUResource::Release() の中で済む
		// (m_srvHandle は基底が持っていて、返したあと空にされる)
		GPUResource::Release();
		m_gpuBuffer.Release();

		// UploadFrame 用の区画つきバッファ(使っていなければ作られていない)
		if (m_pFrameUploadMap)
		{
			m_frameUploadBuffer.Unmap();
			m_pFrameUploadMap = nullptr;
		}
		m_frameUploadBuffer.Release();
	}
	bool StaticBuffer::Create(
		D3D12::Device* a_pDevice, 
		DescriptorHeapManager* a_pHeapManager,
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
		if (!DynamicBuffer::Create(a_pDevice, a_pHeapManager, _desc))
		{
			assert(0 && "リソース作成失敗");
			return false;
		}

		//------------------------------------------------------------------------------------------
		// アップロード側のSRVはここで返す
		//
		// DynamicBuffer::Create はアップロードバッファにもSRVを取るが、
		// StaticBuffer がシェーダーへ見せるのはGPU側のバッファだけ。
		// 派生クラスの Create は m_srvHandle をGPU側のSRVで上書きするので、
		// ここで返しておかないと、1本作るたびにヒープの席が1つ漏れる。
		// 返却先(m_pHeapManager)は後でGPU側のSRVを返すのに使うので残しておく
		//------------------------------------------------------------------------------------------
		if (m_pHeapManager)
		{
			m_pHeapManager->Free(m_srvHandle);
		}
		m_srvHandle = {};

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

		// 範囲外なら CopyBufferRegion も積まない。
		// UpdateDataOffset 側だけ弾いてコピーを積むと、リソース外を指したコピーで
		// デバイスロストになり、これも原因が追いにくい形で表面化する
		if (a_destOffsetBytes + a_sizeBytes > GetBufferSize())
		{
			assert(0 && "バッファサイズを超える部分更新 : Createの要素数が足りていない");
			return;
		}

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

	void StaticBuffer::UploadFrame(GraphicsCommandList* a_pCmdList, const void* a_pData, size_t a_sizeBytes, UINT a_frameIndex)
	{
		if (!a_pCmdList || !a_pData || a_sizeBytes == 0) return;

		// 1区画はGPUバッファと同じ大きさ。超えると隣のフレームの区画を踏む
		const size_t _slotSize = GetBufferSize();
		if (a_sizeBytes > _slotSize)
		{
			assert(0 && "バッファサイズを超える書き込み : Createの要素数が足りていない");
			return;
		}
		assert(a_frameIndex < CPU_FRAME_COUNT && "UploadFrame : フレーム番号が範囲外です");

		if (!m_pFrameUploadMap && !CreateFrameUploadBuffer()) return;

		// 今のフレームの区画へ書く。
		// この区画を最後に読んだのは CPU_FRAME_COUNT フレーム前のコピーで、
		// フレームの頭(FrameManager::BeginFrame)でその完了はもう待ってある
		const size_t _offset = _slotSize * (a_frameIndex % CPU_FRAME_COUNT);
		std::memcpy(m_pFrameUploadMap + _offset, a_pData, a_sizeBytes);

		// 書いたぶんだけGPUバッファへ写す
		m_gpuBuffer.Barrier(a_pCmdList, D3D12_RESOURCE_STATE_COPY_DEST);
		a_pCmdList->CopyBufferRegion(
			m_gpuBuffer.GetResource(),
			0,
			m_frameUploadBuffer.GetResource(),
			_offset,
			a_sizeBytes
		);
		m_gpuBuffer.Barrier(a_pCmdList, D3D12_RESOURCE_STATE_COMMON);
	}

	bool StaticBuffer::CreateFrameUploadBuffer()
	{
		// 作成時と同じデバイスをリソースから引く(呼び出し側に持ち回らせないため)
		if (!m_cpResource) return false;

		ComPtr<D3D12::Device> _cpDevice = nullptr;
		if (FAILED(m_cpResource->GetDevice(IID_PPV_ARGS(_cpDevice.ReleaseAndGetAddressOf()))))
		{
			ENGINE_ERRLOG(false, "UploadFrame : デバイスを取得できませんでした");
			return false;
		}

		GPUBufferDesc _desc = {};
		_desc.strideSize = m_strideSize;
		_desc.elementNum = m_elementNum * CPU_FRAME_COUNT;
		_desc.heapType = D3D12_HEAP_TYPE_UPLOAD;
		_desc.flags = D3D12_RESOURCE_FLAG_NONE;
		if (!m_frameUploadBuffer.Create(_cpDevice.Get(), _desc))
		{
			ENGINE_ERRLOG(false, "UploadFrame : 区画つきアップロードバッファの作成に失敗しました");
			return false;
		}

		void* _pMap = nullptr;
		m_frameUploadBuffer.Map(&_pMap);
		m_pFrameUploadMap = static_cast<std::byte*>(_pMap);
		return m_pFrameUploadMap != nullptr;
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

	void StaticBuffer::CreateSRVInternal(D3D12::Device* a_pDevice, DescriptorHeapManager* a_pHeapManager)
	{
		if (!a_pHeapManager)
		{
			ENGINE_ERRLOG(false, "SRVの確保先ディスクリプタヒープが渡されていません");
			return;
		}

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
		m_pHeapManager = a_pHeapManager;
		m_srvHandle = a_pHeapManager->Allocate<SRV>(a_pDevice, m_gpuBuffer.GetResource(), &_desc);
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