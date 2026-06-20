#pragma once
namespace Engine::D3D12
{
	// 優先度順デバイスメーカー
	enum class GPUTier
	{
		NVIDIA,
		Amd,
		Intel,
		Arm,
		Qualcomm,
		Kind,
	};

	// デバイス関係
	using Device				= ID3D12Device5;					// GPUインスタンス
	using SwapChain				= IDXGISwapChain4;					// スワップチェイン
	using Adapter				= IDXGIAdapter;						// GPU
	using Factory				= IDXGIFactory6;					// ファクトリー

	// コマンド関係
	using CommandQueue			= ID3D12CommandQueue;				// コマンドキュー
	using GraphicsCommandList	= ID3D12GraphicsCommandList4;		// コマンドリスト

	// フェンス
	using Fence					= ID3D12Fence;						// フェンス

	// 描画用構造体
	using Viewport				= D3D12_VIEWPORT;					// 描画用画面領域
	using ScissorRect			= D3D12_RECT;						// 描画する用のサイズ
}