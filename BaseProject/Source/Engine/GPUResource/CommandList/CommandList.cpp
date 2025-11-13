#include "CommandList.h"

bool CommandList::Init(
	ID3D12Device8* a_pDevice, 
	ID3D12CommandAllocator* a_pCommandAllocator, 
	UINT a_currentBackBufferIndex,
	D3D12_COMMAND_LIST_TYPE a_commandListType
)
{
	if (!CreateCommandList(a_pDevice, a_pCommandAllocator, a_commandListType))
	{
		printf("コマンドリストの生成に失敗\n");
		return false;
	}
	return true;
}

bool CommandList::Reset(ID3D12CommandAllocator* a_pCommandAllocator)
{
	m_pCommandList->Reset(a_pCommandAllocator,nullptr);

	return true;
}

bool CommandList::CreateCommandList(
	ID3D12Device8* a_pDevice, 
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
		IID_PPV_ARGS(&m_pCommandList)
	);
	if (FAILED(_hr))
	{
		return false;
	}

	// コマンドリストは生成直後は記録状態になっているので閉じておく
	m_pCommandList->Close();

	return true;
}

