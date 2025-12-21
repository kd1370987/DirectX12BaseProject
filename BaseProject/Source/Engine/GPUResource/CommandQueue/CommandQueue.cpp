#include "CommandQueue.h"

bool CommandQueue::Init(
	ID3D12Device8* a_pDevice, 
	D3D12_COMMAND_LIST_TYPE a_type, 
	INT a_priority, 
	D3D12_COMMAND_QUEUE_FLAGS a_flags
)
{
	// 仕様書生成
	D3D12_COMMAND_QUEUE_DESC _desc = {};
	_desc.Type = a_type;					// コマンドリストの種類を決定
	_desc.Priority = a_priority;			// 優先度
	_desc.Flags = a_flags;					// 追加オプション
	_desc.NodeMask = 0;						// マルチGPU環境なら変更(基本シングルなので0or1)

	// コマンドキュー生成
	if (!CreateCommandQueue(a_pDevice, _desc))
	{
		assert(0 && "コマンドキューの生成に失敗");
		return false;
	}
	return true;
}

bool CommandQueue::CreateCommandQueue(ID3D12Device8* a_pDevice, D3D12_COMMAND_QUEUE_DESC a_desc)
{
	// コマンドキュー生成
	auto _hr = a_pDevice->CreateCommandQueue(
		&a_desc, 
		IID_PPV_ARGS(m_pCommandQueue.ReleaseAndGetAddressOf())
	);

	// 生存チェック・リターン（成功ならS_OK(0)を返す）
	return SUCCEEDED(_hr);
}
