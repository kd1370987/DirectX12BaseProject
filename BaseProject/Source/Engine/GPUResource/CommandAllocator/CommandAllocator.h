#pragma once

class CommandAllocator
{
public:

	// コンストラクタ・デストラクタ
	CommandAllocator() = default;
	~CommandAllocator() = default;

	// コマンドアロケーターの初期化
	bool Init(
		ID3D12Device8* a_pDevice,
		UINT a_frameBufferCount,
		D3D12_COMMAND_LIST_TYPE a_commandListType
	);

	// リセット
	void Reset(UINT a_frameIdx);

	// コマンドアロケーターの取得
	ID3D12CommandAllocator* GetCCurrentAllocator(UINT a_frameIdx) 
	{
		return m_pCommandAllocatorVec[a_frameIdx].Get(); 
	}

private:

	// コマンドアロケーターの生成
	bool CreateCommandAllocator(
		ID3D12Device8* a_pDevice,
		UINT a_frameBufferCount,
		D3D12_COMMAND_LIST_TYPE a_commandListType
	);

private:

	// コマンドアロケーター群
	std::vector<ComPtr<ID3D12CommandAllocator>> m_pCommandAllocatorVec;
};