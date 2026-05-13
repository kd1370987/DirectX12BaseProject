#pragma once
namespace Engine::D3D12
{
	class CommandList
	{
	public:
		// コンストラクタ・デストラクタ
		CommandList() {}
		~CommandList() {}

		/// <summary>
		/// コマンドリスト生成
		/// </summary>
		/// <param name="a_pDevice">アダプタ</param>
		/// <param name="a_pCommandAllocator">コマンドアロケーター</param>
		/// <param name="a_currentBackBufferIndex">現在のバックバッファインデックス</param>
		/// <param name="a_commandListType">コマンドリストタイプ</param>
		/// <returns>成功 = true</returns>
		bool Create(
			ID3D12Device* a_pDevice,
			ID3D12CommandAllocator* a_pCommandAllocator,
			D3D12_COMMAND_LIST_TYPE a_commandListType = D3D12_COMMAND_LIST_TYPE_DIRECT
		);

		/// <summary>
		/// コマンドリストリセット
		/// </summary>
		/// <param name="a_pCommandAllocator">コマンドアロケーター</param>
		/// <returns>成功 = true</returns>
		void Reset(ID3D12CommandAllocator* a_pCommandAllocator);

		/// <summary>
		/// コマンドリストのネイティブ取得
		/// </summary>
		/// <returns>生ポインタ</returns>
		ID3D12GraphicsCommandList* NGet() { return m_cpCommandList.Get(); }

		ID3D12GraphicsCommandList4* Get4() { return m_cpCommandList.Get(); }

		/// <summary>
		/// リソースバリアを張る
		/// </summary>
		/// <param name="a_pResource">対象のリソース</param>
		/// <param name="a_before">前の状態</param>
		/// <param name="a_after">後の状態</param>
		void ResourceBarrier(
			ID3D12Resource* a_pResource,
			D3D12_RESOURCE_STATES a_before,
			D3D12_RESOURCE_STATES a_after
		);

		/// <summary>
		/// コマンドの受付を終了する
		/// </summary>
		void Close();

		/// <summary>
		/// ビューポート指定
		/// </summary>
		/// <param name="a_num">ビューポート数</param>
		/// <param name="a_pViewports">ビューポートポインタ</param>
		void SetViewports(
			UINT a_num,
			const D3D12_VIEWPORT* a_pViewports
		);

		/// <summary>
		/// シザー矩形指定
		/// </summary>
		/// <param name="a_num">シザー矩形個数</param>
		/// <param name="a_pScissorRect">シザー矩形ポインタ</param>
		void SetScissorRects(
			UINT a_num,
			const D3D12_RECT* a_pScissorRect
		);


		// ディスクリプタヒープをバインド
		void SetGraphicsRootDescriptorTable(
			UINT a_rootIdx,
			D3D12_GPU_DESCRIPTOR_HANDLE a_baseHandle
		);

		// ディスクリプタヒープのセット
		void SetDescriptorHeaps(
			UINT a_numHeaps,
			ID3D12DescriptorHeap* const* a_pHeaps
		);

		// ルートシグネチャをセット
		void SetGraphicsRootSignature(ID3D12RootSignature* a_pRootSig);		// グラフィック
		void SetComputeRootSignature(ID3D12RootSignature* a_pRootSignature);// コンピュート

		// パイプラインステートをセット
		void SetPipelineState1(ID3D12StateObject* a_pStateObject);

		// コンピュートにSRVをセット
		void SetComputeRootShaderResourceView(UINT a_rootParamIdx,D3D12_GPU_VIRTUAL_ADDRESS a_location);

		// ディスクリプタテーブルのセット
		void SetComputeRootDescriptorTable(UINT a_rootParamIdx,D3D12_GPU_DESCRIPTOR_HANDLE a_baseDescriptor);

		// ディスパッチレイ
		void DispatchRays(const D3D12_DISPATCH_RAYS_DESC* a_pDesc);
		
		// 頂点バッファセット
		void IASetVertexBuffers(UINT a_startSlot,UINT a_numViews, const D3D12_VERTEX_BUFFER_VIEW* a_pViews);

		// インデックスバッファセット
		void IASetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW* a_pViews);


		/// <summary>
		/// レンダーターゲットをセット
		/// <summary>
		/// <param name="a_numRenderTargetDescriptors">レンダーターゲット数</param>
		/// <param name="a_pRenderTargetDescriptors">レンダーターゲットハンドル</param>
		/// <param name="a_RTsSingleHandleToDescriptorRange"></param>
		/// <param name="a_pDepthStencilDescriptor">深度ステンシルバッファハンドル</param>
		void OMSetRenderTargets(
			UINT a_numRenderTargetDescriptors,
			const D3D12_CPU_DESCRIPTOR_HANDLE* a_pRenderTargetDescriptors,
			BOOL a_RTsSingleHandleToDescriptorRange,
			const D3D12_CPU_DESCRIPTOR_HANDLE* a_pDepthStencilDescriptor
		);

		/// <summary>
		/// レンダーターゲットをクリア
		/// </summary>
		/// <param name="a_renderTargetView">指定レンダーターゲット</param>
		/// <param name="a_colorRGBA">レンダーターゲットのクリアカラー</param>
		/// <param name="a_numRects">矩形数</param>
		/// <param name="a_pRects">矩形ポインタ</param>
		void ClearRenderTargetView(
			D3D12_CPU_DESCRIPTOR_HANDLE a_renderTargetView,
			DirectX::XMFLOAT4 a_colorRGBA = { 0.0f,0.0f,0.0f,1.0f },
			UINT a_numRects = 0,
			const D3D12_RECT* a_pRects = nullptr
		);


		/// <summary>
		/// 深度ステンシルビューをクリア
		/// </summary>
		/// <param name="a_depthStencilView">指定対象のハンドル</param>
		/// <param name="a_clearFlags">クリアフラグ</param>
		/// <param name="a_depth">奥(1.0)</param>
		/// <param name="a_stencil">手前(0.0)</param>
		/// <param name="a_numRects">矩形数</param>
		/// <param name="a_pRects">矩形ポインタ</param>
		void ClearDepthStencilView(
			D3D12_CPU_DESCRIPTOR_HANDLE a_depthStencilView,
			D3D12_CLEAR_FLAGS a_clearFlags = D3D12_CLEAR_FLAG_DEPTH,
			float a_depth = 1.0f,
			float a_stencil = 0.0f,
			UINT a_numRects = 0,
			const D3D12_RECT* a_pRects = nullptr
		);

		// 特定のリソースからリソースへコピーする
		void CopyBufferRegion(
			ID3D12Resource* a_pDstBuffer,
			UINT64 a_dstOffset,
			ID3D12Resource* a_pSrcBuffer,
			UINT64 a_srcOffset,
			UINT64 a_numBytes
		);

	private:

		ComPtr<ID3D12GraphicsCommandList4> m_cpCommandList = nullptr;	// コマンドリスト
	};
}