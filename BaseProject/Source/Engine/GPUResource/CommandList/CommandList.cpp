#include "CommandList.h"

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
	HRESULT _hr = m_cpCommandList->Reset(a_pCommandAllocator,nullptr);
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
	m_cpCommandList->RSSetScissorRects(a_num,a_pScissorRect);
}

void CommandList::SetRenderTarget(
	UINT a_numRenderTargetDescriptors,
	const D3D12_CPU_DESCRIPTOR_HANDLE* a_pRenderTargetDescriptors,
	BOOL a_RTsSingleHandleToDescriptorRange, 
	const D3D12_CPU_DESCRIPTOR_HANDLE* a_pDepthStencilDescriptor
)
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
		a_stencil,
		a_numRects,
		a_pRects
	);
}


