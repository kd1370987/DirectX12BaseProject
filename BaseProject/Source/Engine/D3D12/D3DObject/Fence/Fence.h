#pragma once
namespace Engine::D3D12
{
	class Fence
	{
	public:

		Fence() = default;
		~Fence() = default;

		/// <summary>
		/// フェンスの生成
		/// </summary>
		/// <param name="ID3D12Device*">GPU/アダプタ</param>
		/// <returns>成功 = true</returns>
		bool Create(
			ID3D12Device* a_pDevice
		);

		ID3D12Fence* GetFence() { return m_pFence.Get(); }
		UINT64 GetCompletedValue() { return m_pFence->GetCompletedValue(); }

		/// <summary>
		/// イベント生成
		/// </summary>
		/// <param name="a_fenceValue">フェンスの値</param>
		/// <param name="a_fenceEvent">イベント</param>
		/// <returns></returns>
		bool SetEventOnCompletion(UINT64 a_fenceValue, HANDLE a_fenceEvent);

	private:

		// フェンス
		ComPtr<ID3D12Fence1> m_pFence = nullptr;

	};
}