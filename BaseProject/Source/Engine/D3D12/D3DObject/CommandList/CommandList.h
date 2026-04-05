#pragma once

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

	/// <summary>
	/// レンダーターゲットをセット
	/// </summary>
	/// <param name="a_numRenderTargetDescriptors">レンダーターゲット数</param>
	/// <param name="a_pRenderTargetDescriptors">レンダーターゲットハンドル</param>
	/// <param name="a_RTsSingleHandleToDescriptorRange"></param>
	/// <param name="a_pDepthStencilDescriptor">深度ステンシルバッファハンドル</param>
	void SetRenderTarget(
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
		DirectX::XMFLOAT4 a_colorRGBA = {0.0f,0.0f,0.0f,1.0f},
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

private:

	ComPtr<ID3D12GraphicsCommandList4> m_cpCommandList = nullptr;	// コマンドリスト
};