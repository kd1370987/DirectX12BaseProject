#pragma once
namespace Engine::Editor
{
	struct Timer;

	/// <summary>
	/// GPUの実行時間などを図るためのもの
	/// </summary>
	class GPUProfiler
	{
	public:

		//-------------------------------------------------------------------------------------
		// 全体設定
		//-------------------------------------------------------------------------------------
		// 初期化
		void Init();

		// 更新
		void BeginFrame();
		void EndFrame();

		//-------------------------------------------------------------------------------------
		// 個別設定
		//-------------------------------------------------------------------------------------
		void Start(D3D12::GraphicsCommandList* a_pCmdList, Timer& a_timer);
		void End(D3D12::GraphicsCommandList* a_pCmdList, Timer& a_timer);

	private:
		ComPtr<ID3D12QueryHeap> m_upQueryHeap;

		D3D12::GPUBuffer m_readBackBuffer;

		uint32_t m_nextQuery = 0;
	};
}