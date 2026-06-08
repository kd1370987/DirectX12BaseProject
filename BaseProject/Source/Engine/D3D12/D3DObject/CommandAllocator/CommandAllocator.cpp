#include "CommandAllocator.h"
namespace Engine::D3D12
{


	void CommandAllocator::Reset()
	{
		m_cpCommandAllocator->Reset();
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
	void CommandAllocator::Release()
	{
		m_cpCommandAllocator.Reset();
	}
}