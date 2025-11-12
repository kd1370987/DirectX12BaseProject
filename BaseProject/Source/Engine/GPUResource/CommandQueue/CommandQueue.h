#pragma once

class CommandQueue
{
public:

	CommandQueue(){}
	~CommandQueue(){}

	// コマンドキュー初期化
	// a_pDevice	: デバイス
	// a_type		: コマンドリストの種類
	// a_priority	: 優先度
	// a_flags		: 追加オプション
	bool Init(
		ID3D12Device8* a_pDevice,
		D3D12_COMMAND_LIST_TYPE a_type = D3D12_COMMAND_LIST_TYPE_DIRECT,
		INT a_priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
		D3D12_COMMAND_QUEUE_FLAGS a_flags = D3D12_COMMAND_QUEUE_FLAG_NONE
	);

	// コマンドキュー取得
	ID3D12CommandQueue* Get() const { return m_pCommandQueue.Get(); }

private:

	bool CreateCommandQueue(ID3D12Device8* a_pDevice, D3D12_COMMAND_QUEUE_DESC a_desc);

private:

	ComPtr<ID3D12CommandQueue> m_pCommandQueue = nullptr;

};