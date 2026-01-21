#pragma once

class CommandQueue
{
public:

	CommandQueue() = default;
	~CommandQueue() = default;

	/// <summary>
	/// コマンドキュー作成
	/// </summary>
	/// <param name="a_pDevice">デバイスポインタ</param>
	/// <param name="a_type">コマンドリストのタイプ</param>
	/// <param name="a_priority">優先度</param>
	/// <param name="a_flags">追加オプションフラグ</param>
	/// <returns>成功 = true</returns>
	bool Create(
		ID3D12Device* a_pDevice,
		D3D12_COMMAND_LIST_TYPE a_type = D3D12_COMMAND_LIST_TYPE_DIRECT,
		INT a_priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
		D3D12_COMMAND_QUEUE_FLAGS a_flags = D3D12_COMMAND_QUEUE_FLAG_NONE
	);

	/// <summary>
	/// コマンドキュー取得
	/// </summary>
	ID3D12CommandQueue* Get() const { return m_cpCommandQueue.Get(); }

private:

	ComPtr<ID3D12CommandQueue> m_cpCommandQueue = nullptr;

};