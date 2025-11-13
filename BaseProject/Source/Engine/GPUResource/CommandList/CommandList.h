#pragma once

class CommandList
{
public:
	// コンストラクタ・デストラクタ
	CommandList() {}
	~CommandList() {}

	// コマンドリストの初期化
	bool Init(
		ID3D12Device8* a_pDevice,
		ID3D12CommandAllocator* a_pCommandAllocator,
		UINT a_currentBackBufferIndex,
		D3D12_COMMAND_LIST_TYPE a_commandListType = D3D12_COMMAND_LIST_TYPE_DIRECT
	);

	// コマンドリストリセット
	bool Reset(ID3D12CommandAllocator* a_pCommandAllocator);

	// コマンドリストの取得
	ID3D12GraphicsCommandList* GetCommandList() { return m_pCommandList.Get(); }

private:

	// コマンドリストの生成
	bool CreateCommandList(
		ID3D12Device8* a_pDevice,
		ID3D12CommandAllocator* a_pCommandAllocator,
		D3D12_COMMAND_LIST_TYPE a_commandListType
	);

private:

	ComPtr<ID3D12GraphicsCommandList> m_pCommandList = nullptr;	// コマンドリスト

};