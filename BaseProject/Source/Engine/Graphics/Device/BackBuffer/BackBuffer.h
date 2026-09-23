#pragma once

namespace Engine::D3D12
{
	class DescriptorHeapManager;
}
namespace Engine::Graphics
{
	//==========================================================================================
	// バックバッファ(スワップチェイン・画面へ出すテクスチャ・ビューポート)
	//
	// GraphicsEngine が持つ。
	// RTVをディスクリプタヒープに預けているので、ヒープより先に Release() すること。
	//
	// ステートの遷移はここの関数か、RefBackBuffer() の Barrier() を通すこと。
	// テクスチャは自分のステートを覚えていて、生の ResourceBarrier で遷移させると
	// 覚えている値だけが取り残され、次の Barrier() が何もしなくなる
	//==========================================================================================
	class BackBuffer
	{
	public:

		BackBuffer() = default;
		~BackBuffer() { Release(); }

		// バックバッファの作成 : スワップチェインやビューポート関連もできる。
		// スワップチェインは描画キューに紐づくので、キューを作った後に呼ぶ
		void Create(
			D3D12::DescriptorHeapManager* a_pHeapManager,
			D3D12::Factory* a_pFactory,
			D3D12::CommandQueue* a_pCmdQueue,
			HWND a_hWnd, UINT a_windowWidth, UINT a_windowHeight
		);

		// 解放 : 二度呼んでも何もしない(デストラクタからも通る)
		void Release();

		// ---- 更新 ----
		void BeginFrame();													// フレームの初めに呼び出す
		void TransitionToRenderTarget(D3D12::GraphicsCommandList* a_pCmd);	// 描き込める状態にする
		void TransitionToPresent(D3D12::GraphicsCommandList* a_pCmd);		// 書き込みが終わるのをプレゼント状態にして待つ
		void Present(bool a_isVsync, UINT a_flag = 0);						// スワップチェインの切り替え

		// ---- アクセサ ----
		UINT GetCurrentIndex() const { return m_currentIndex; }

		const Resource::Texture& GetBackBuffer()const { return m_backBuffers[m_currentIndex]; }
		const Resource::Texture& GetBackBuffer(UINT a_index) const { return m_backBuffers[a_index]; }

		// コピー先にするなど、ステートを変えたいときに使う
		Resource::Texture& RefBackBuffer() { return m_backBuffers[m_currentIndex]; }

		const D3D12::Viewport& GetViewport() const { return m_viewport; }
		const D3D12::ScissorRect& GetScissorRect() const { return m_scissorRect; }

	private:

		// 各初期化データ作成
		void CreateSwapChain(														// スワップチェイン作成
			D3D12::Factory* a_pFactory,
			D3D12::CommandQueue* a_pCmdQueue,
			HWND a_hWnd, UINT a_windowWidth, UINT a_windowHeight
		);
		void CreateViewPort(UINT a_windowWidth, UINT a_windowHeight);				// 描画用領域設定
		void CreateScissorRect(UINT a_windowWidth, UINT a_windowHeight);			// 描画範囲作成

	private:
		// リソース本体
		std::array<Resource::Texture, BACKBUFFER_COUNT> m_backBuffers;

		// スワップチェイン
		ComPtr<D3D12::SwapChain> m_cpSwapChain = nullptr;

		// バックバッファ情報
		BOOL m_isAllowTearing = FALSE;			// フレーム途中に次の画像が割り込んでもいいか
		UINT m_currentIndex = 0;				// 現在のバックバッファのインデックス

		// 表示領域
		D3D12::Viewport m_viewport;
		D3D12::ScissorRect m_scissorRect;
	};
}
