#pragma once

class CommandAllocator
{
public:

	// コンストラクタ・デストラクタ
	CommandAllocator() = default;
	~CommandAllocator() = default;

	/// <summary>
	/// コマンドアロケーターの生成
	/// </summary>
	/// <param name="a_pDevice">デバイスのポインタ</param>
	/// <param name="a_frameBufferCount">生成数</param>
	/// <param name="a_commandListType">コマンドリストのタイプ</param>
	/// <returns>成功 = true</returns>
	bool Create(
		ID3D12Device* a_pDevice,
		UINT a_frameBufferCount,
		D3D12_COMMAND_LIST_TYPE a_commandListType
	);

	// 単体生成
	bool Create(
		ID3D12Device* a_pDevice,
		D3D12_COMMAND_LIST_TYPE a_commandListType
	);


	// リセット
	void Reset(UINT a_frameIdx);
	void Reset();

	// コマンドアロケーターの取得
	ID3D12CommandAllocator* Get(UINT a_frameIdx) 
	{
		return m_pCommandAllocatorVec[a_frameIdx].Get(); 
	}
	ID3D12CommandAllocator* Get()
	{
		return m_cpCommandAllocator.Get();
	}

private:

	// コマンドアロケーター群
	std::vector<ComPtr<ID3D12CommandAllocator>> m_pCommandAllocatorVec;

	ComPtr<ID3D12CommandAllocator> m_cpCommandAllocator = nullptr;
};