#include "GPUResource.h"

#include "../../DescriptorHeapManager/DescriptorHeapManager.h"

namespace Engine::D3D12
{
	bool GPUResource::Create(D3D12::Device* a_pDevice, const GPUResourceDesc& a_desc)
	{
		// リソースサイズ計算
		m_strideSize = a_desc.strideSize;
		m_elementNum = a_desc.elementNum;
		m_bufferSize = m_strideSize * m_elementNum;

		// ステート設定
		m_currentState = a_desc.farstState;
		
		// ヒーププロパティ生成
		auto _prop = CD3DX12_HEAP_PROPERTIES(a_desc.heapType);

		// リソース生成
		auto _hr = a_pDevice->CreateCommittedResource(
			&_prop,
			a_desc.heapFlags,
			&a_desc.resourceDesc,
			m_currentState,
			a_desc.pClearValue,						// バッファだとnullptrなので
			IID_PPV_ARGS(m_cpResource.ReleaseAndGetAddressOf())
		);
		if (FAILED(_hr))
		{
			ENGINE_ERRLOG(false,"リソース生成に失敗");
			return false;
		}

		// フォーマットセット
		m_format = a_desc.format;

		// リーク調査用 : ライブオブジェクトレポートに種別と通し番号を出すため名前を付ける。
		// (要素サイズ/数も入れておくと、どのバッファか特定しやすい)
		{
			static std::atomic<uint32_t> s_counter = 0;
			wchar_t _name[128];
			swprintf_s(_name, L"GPUResource#%u (stride=%zu num=%zu)",
				s_counter.fetch_add(1), a_desc.strideSize, a_desc.elementNum);
			m_cpResource->SetName(_name);
		}

		// 成功
		return true;
	}
	void GPUResource::Release()
	{
		m_cpResource.Reset();

		DescriptorHeapManager::Instance().Free(m_srvHandle);
		DescriptorHeapManager::Instance().Free(m_uavHandle);
		DescriptorHeapManager::Instance().Free(m_rtvHandle);
		DescriptorHeapManager::Instance().Free(m_dsvHandle);
		DescriptorHeapManager::Instance().Free(m_readOnlyDsvHandle);
		DescriptorHeapManager::Instance().Free(m_imguiSRVHandle);

	}
	void GPUResource::Barrier(D3D12::GraphicsCommandList* a_pCmdList, D3D12_RESOURCE_STATES a_nextState)
	{
		if (m_currentState == a_nextState) return;

		D3D12_RESOURCE_BARRIER _barrier = {};
		_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		_barrier.Transition.pResource = m_cpResource.Get();
		_barrier.Transition.StateAfter = a_nextState;
		_barrier.Transition.StateBefore = m_currentState;
		_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		a_pCmdList->ResourceBarrier(1, &_barrier);

		// ステートの更新
		m_currentState = a_nextState;
	}
	ID3D12Resource* GPUResource::GetResource() const
	{
		return m_cpResource.Get();
	}
	D3D12_GPU_VIRTUAL_ADDRESS GPUResource::GetGPUVirtualAddress() const
	{
		return m_cpResource->GetGPUVirtualAddress();
	}
	size_t GPUResource::GetBufferSize() const
	{
		return m_bufferSize;
	}
	size_t GPUResource::GetStrideSize() const
	{
		return m_strideSize;
	}
	size_t GPUResource::GetElementNum() const
	{
		return m_elementNum;
	}
}