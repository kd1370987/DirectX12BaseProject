#include "CommandList.h"
namespace Engine::D3D12
{
	bool CommandList::Create(
		ID3D12Device* a_pDevice,
		ID3D12CommandAllocator* a_pCommandAllocator,
		D3D12_COMMAND_LIST_TYPE a_commandListType
	)
	{
		// コマンドリストの生成
		HRESULT _hr = a_pDevice->CreateCommandList(
			0,
			a_commandListType,
			a_pCommandAllocator,
			nullptr,
			IID_PPV_ARGS(&m_cpCommandList)
		);
		if (FAILED(_hr))
		{
			assert(0 && "コマンドリストの生成に失敗");
			return false;
		}

		// コマンドリストは生成直後は記録状態になっているので閉じておく
		m_cpCommandList->Close();

		return true;
	}

	void CommandList::Reset(ID3D12CommandAllocator* a_pCommandAllocator)
	{
		HRESULT _hr = m_cpCommandList->Reset(a_pCommandAllocator, nullptr);
		if (FAILED(_hr))
		{
			assert(0 && "コマンドリストリセット失敗");
		}
	}

	void CommandList::ResourceBarrier(ID3D12Resource* a_pResource, D3D12_RESOURCE_STATES a_before, D3D12_RESOURCE_STATES a_after)
	{
		D3D12_RESOURCE_BARRIER _barrier = {};
		_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		_barrier.Transition.pResource = a_pResource;
		_barrier.Transition.StateAfter = a_after;
		_barrier.Transition.StateBefore = a_before;
		_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		m_cpCommandList->ResourceBarrier(1, &_barrier);
	}

	void CommandList::Close() {
		m_cpCommandList->Close();
	}

	void CommandList::SetViewports(UINT a_num, const D3D12_VIEWPORT* a_pViewports)
	{
		m_cpCommandList->RSSetViewports(a_num, a_pViewports);
	}

	void CommandList::SetScissorRects(UINT a_num, const D3D12_RECT* a_pScissorRect)
	{
		m_cpCommandList->RSSetScissorRects(a_num, a_pScissorRect);
	}

	
	void CommandList::SetGraphicsRootDescriptorTable(UINT a_rootIdx, D3D12_GPU_DESCRIPTOR_HANDLE a_baseHandle)
	{
		m_cpCommandList->SetGraphicsRootDescriptorTable(
			a_rootIdx,
			a_baseHandle
		);
	}

	void CommandList::SetDescriptorHeaps(UINT a_numHeaps, ID3D12DescriptorHeap* const* a_pHeaps)
	{
		m_cpCommandList->SetDescriptorHeaps(a_numHeaps,a_pHeaps);
	}

	void CommandList::SetGraphicsRootSignature(ID3D12RootSignature* a_pRootSig)
	{
		m_cpCommandList->SetGraphicsRootSignature(a_pRootSig);
	}

	void CommandList::SetComputeRootSignature(ID3D12RootSignature* a_pRootSignature)
	{
		m_cpCommandList->SetComputeRootSignature(a_pRootSignature);
	}

	void CommandList::SetPipelineState1(ID3D12StateObject* a_pStateObject)
	{
		m_cpCommandList->SetPipelineState1(a_pStateObject);
	}

	void CommandList::SetComputeRootShaderResourceView(UINT a_rootParamIdx, D3D12_GPU_VIRTUAL_ADDRESS a_location)
	{
		m_cpCommandList->SetComputeRootShaderResourceView(a_rootParamIdx,a_location);
	}

	void CommandList::SetComputeRootDescriptorTable(UINT a_rootParamIdx, D3D12_GPU_DESCRIPTOR_HANDLE a_baseDescriptor)
	{
		m_cpCommandList->SetComputeRootDescriptorTable(a_rootParamIdx,a_baseDescriptor);
	}

	void CommandList::DispatchRays(const D3D12_DISPATCH_RAYS_DESC* a_pDesc)
	{
		m_cpCommandList->DispatchRays(a_pDesc);
	}

	void CommandList::IASetVertexBuffers(UINT a_startSlot, UINT a_numViews, const D3D12_VERTEX_BUFFER_VIEW* a_pViews)
	{
		m_cpCommandList->IASetVertexBuffers(a_startSlot,a_numViews,a_pViews);
	}

	void CommandList::IASetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW* a_pViews)
	{
		m_cpCommandList->IASetIndexBuffer(a_pViews);
	}

	void CommandList::OMSetRenderTargets(UINT a_numRenderTargetDescriptors, const D3D12_CPU_DESCRIPTOR_HANDLE* a_pRenderTargetDescriptors, BOOL a_RTsSingleHandleToDescriptorRange, const D3D12_CPU_DESCRIPTOR_HANDLE* a_pDepthStencilDescriptor)
	{
		m_cpCommandList->OMSetRenderTargets(
			a_numRenderTargetDescriptors,
			a_pRenderTargetDescriptors,
			a_RTsSingleHandleToDescriptorRange,
			a_pDepthStencilDescriptor
		);
	}

	void CommandList::ClearRenderTargetView(
		D3D12_CPU_DESCRIPTOR_HANDLE a_renderTargetView,
		DirectX::XMFLOAT4 a_colorRGBA,
		UINT a_numRects,
		const D3D12_RECT* a_pRects
	)
	{
		const float _clearColor[] = { a_colorRGBA.x	,a_colorRGBA.y, a_colorRGBA.z, a_colorRGBA.w };
		m_cpCommandList->ClearRenderTargetView(
			a_renderTargetView,
			_clearColor,
			a_numRects,
			a_pRects
		);
	}

	void CommandList::ClearDepthStencilView(
		D3D12_CPU_DESCRIPTOR_HANDLE a_depthStencilView,
		D3D12_CLEAR_FLAGS a_clearFlags,
		float a_depth,
		float a_stencil,
		UINT a_numRects,
		const D3D12_RECT* a_pRects
	)
	{
		m_cpCommandList->ClearDepthStencilView(
			a_depthStencilView,
			a_clearFlags,
			a_depth,
			static_cast<UINT8>(a_stencil),
			a_numRects,
			a_pRects
		);
	}

	void CommandList::CopyBufferRegion(ID3D12Resource* a_pDstBuffer, UINT64 a_dstOffset, ID3D12Resource* a_pSrcBuffer, UINT64 a_srcOffset, UINT64 a_numBytes)
	{
		m_cpCommandList->CopyBufferRegion(
			a_pDstBuffer,
			a_dstOffset,
			a_pSrcBuffer,
			a_srcOffset,
			a_numBytes
		);
	}


}