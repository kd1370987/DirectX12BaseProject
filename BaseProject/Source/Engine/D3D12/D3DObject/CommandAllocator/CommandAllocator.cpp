#include "CommandAllocator.h"

void CommandAllocator::Reset(UINT a_frameIdx)
{
	if (m_pCommandAllocatorVec[a_frameIdx])
	{
		m_pCommandAllocatorVec[a_frameIdx]->Reset();
	}
}

bool CommandAllocator::Create(
	ID3D12Device* a_pDevice,
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
		// コマンドアロケーターの生成
		_hr = a_pDevice->CreateCommandAllocator(
			a_commandListType,
			IID_PPV_ARGS(m_pCommandAllocatorVec[_i].ReleaseAndGetAddressOf())
		);

		// 失敗チェック
		if (FAILED(_hr))
		{
			assert(0 && "コマンドアロケーターの生成に失敗");
			return false;
		}
	}
	return true;
}

bool CommandAllocator::Create(ID3D12Device* a_pDevice, D3D12_COMMAND_LIST_TYPE a_commandListType)
{
	// コマンドアロケーターの生成
	HRESULT _hr;
	
	// コマンドアロケーターの生成
	_hr = a_pDevice->CreateCommandAllocator(
		a_commandListType,
		IID_PPV_ARGS(m_cpCommandAllocator.ReleaseAndGetAddressOf())
	);

	// 失敗チェック
	if (FAILED(_hr))
	{
		assert(0 && "コマンドアロケーターの生成に失敗");
		return false;
	}

	return true;
}
