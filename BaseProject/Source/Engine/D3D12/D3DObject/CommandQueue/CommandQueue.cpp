#include "CommandQueue.h"
namespace Engine::D3D12
{
	bool CommandQueue::Create(
		ID3D12Device* a_pDevice,
		D3D12_COMMAND_LIST_TYPE a_type,
		INT a_priority,
		D3D12_COMMAND_QUEUE_FLAGS a_flags
	)
	{
		// 仕様書作成
		D3D12_COMMAND_QUEUE_DESC _desc = {};
		_desc.Type = a_type;					// コマンドリストの種類を決定
		_desc.Priority = a_priority;			// 優先度
		_desc.Flags = a_flags;					// 追加オプション
		_desc.NodeMask = 0;						// マルチGPU環境なら変更(基本シングルなので0or1)

		// コマンドキュー生成
		HRESULT _hr = a_pDevice->CreateCommandQueue(
			&_desc,
			IID_PPV_ARGS(m_cpCommandQueue.ReleaseAndGetAddressOf())
		);
		if (FAILED(_hr))
		{
			assert(0 && "コマンドキューの生成に失敗");
			return false;
		}

		return true;
	}
}