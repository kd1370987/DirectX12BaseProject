#include "Fence.h"

bool Fence::Init(
	ID3D12Device8* a_pDevice
)
{
	if (!CreateFence(a_pDevice))
	{
		return false;
	}
	return true;
}

bool Fence::SetEventOnCompletion(UINT64 a_fenceValue, HANDLE a_fenceEvent)
{
	HRESULT _hr = m_pFence->SetEventOnCompletion(a_fenceValue, a_fenceEvent);
	if (FAILED(_hr))
	{
		return false;
	}
	return true;
}

bool Fence::CreateFence(
	ID3D12Device8* a_pDecice
)
{
	HRESULT _hr = a_pDecice->CreateFence(
		0,
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(m_pFence.ReleaseAndGetAddressOf())
	);
	if (FAILED(_hr))
	{
		assert(0 && "フェンスの作成に失敗");
		return false;
	}
}
