#include "CommandAllocator.h"

bool CommandAllocator::Init(
	ID3D12Device8* a_pDevice, 
	UINT a_frameBufferCount,
	D3D12_COMMAND_LIST_TYPE a_commandListType
)
{
	if (!CreateCommandAllocator(a_pDevice, a_frameBufferCount, a_commandListType))
	{
		assert(0 && "コマンドアロケーターの生成に失敗\n");
		return false;
	}
	return true;
}

void CommandAllocator::Reset(UINT a_frameIdx)
{
	if (m_pCommandAllocatorVec[a_frameIdx])
	{
		m_pCommandAllocatorVec[a_frameIdx]->Reset();
	}
}

bool CommandAllocator::CreateCommandAllocator(
	ID3D12Device8* a_pDevice, 
	UINT a_frameBufferCount, 
	D3D12_COMMAND_LIST_TYPE a_commandListType
)
{
	// コマンドアロケーター群のサイズ設定
	m_pCommandAllocatorVec.resize(a_frameBufferCount);

	// コマンドアロケーターの生成
	HRESULT _hr;
	for (UINT _i = 0; _i < a_frameBufferCount; ++_i)
	{
		// コマンドアロケーターの追加
		m_pCommandAllocatorVec.emplace_back();

		// コマンドアロケーターの生成
		_hr = a_pDevice->CreateCommandAllocator(
			a_commandListType,
			IID_PPV_ARGS(m_pCommandAllocatorVec[_i].ReleaseAndGetAddressOf())
		);

		// 失敗チェック
		if (FAILED(_hr))
		{
			return false;
		}
	}
	return true;
}
