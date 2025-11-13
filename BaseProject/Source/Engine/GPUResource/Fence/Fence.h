#pragma once

class Fence
{
public:

	Fence() = default;
	~Fence() = default;

	/// <summary>
	/// フェンスの初期化
	/// </summary>
	/// <param name="ID3D12Device8*">GPU/アダプタ</param>
	/// <returns>初期化成功でtrueを返す</returns>
	bool Init(
		ID3D12Device8* a_pDevice

	);

	ID3D12Fence* GetFence() { return m_pFence.Get(); }
	UINT64 GetCompletedValue() { return m_pFence->GetCompletedValue(); }
	bool SetEventOnCompletion(UINT64 a_fenceValue, HANDLE a_fenceEvent);
private:

	// フェンス作成
	bool CreateFence(
		ID3D12Device8* a_pDecice
	);

private:

	// フェンス
	ComPtr<ID3D12Fence> m_pFence = nullptr;

};