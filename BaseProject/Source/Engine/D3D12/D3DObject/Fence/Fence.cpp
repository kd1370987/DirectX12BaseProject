#include "Fence.h"
namespace Engine::D3D12
{
	bool Fence::Create(ID3D12Device* a_pDevice)
	{
		HRESULT _hr = a_pDevice->CreateFence(
			0,
			D3D12_FENCE_FLAG_NONE,
			IID_PPV_ARGS(m_pFence.ReleaseAndGetAddressOf())
		);
		if (FAILED(_hr))
		{
			assert(0 && "フェンスの作成に失敗");
			return false;
		}

		return true;
	}

	void Fence::Release()
	{
		m_pFence.Reset();
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
}