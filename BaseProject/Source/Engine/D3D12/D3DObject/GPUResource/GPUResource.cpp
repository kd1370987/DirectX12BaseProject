#include "GPUResource.h"

namespace Engine::D3D12
{
	bool GPUResource::Create(ID3D12Device* a_pDevice, const GPUResourceDesc& a_desc)
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

		// 成功
		return true;
	}
	void GPUResource::Release()
	{
		m_cpResource.Reset();
	}
	void GPUResource::Barrier(ID3D12GraphicsCommandList* a_pCmdList, D3D12_RESOURCE_STATES a_nextState)
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