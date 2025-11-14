#pragma once

#include "../../GPUResource/Device/Device.h"
#include "../../GPUResource/CommandQueue/CommandQueue.h"
#include "../../GPUResource/CommandAllocator/CommandAllocator.h"
#include "../../GPUResource/CommandList/CommandList.h"
#include "../../GPUResource/Fence/Fence.h"

class RootSignature;
class PipelineState;
class DescriptorHeap;

class ModelResource;
class Mesh;

class ConstantBuffer;

class RenderingEngine
{
public:

	// エンジン初期化
	bool Init(HWND a_hWnd, UINT a_windowWidth, UINT a_windowHeight);

	// 描画開始・描画終了
	void BeginRender();
	void EndRender();

	// シンプル描画
	void BeginSimpleRender();
	void EndSimpleRender();

	// 描画（モデル）
	void DrawModel(
		const ModelResource& a_modelResource,
		const DirectX::XMFLOAT4X4& a_worldMat,
		const DirectX::XMFLOAT4& a_colorScale = { 1,1,1,1 },
		const DirectX::XMFLOAT3& a_emissive = { 1,1,1 }
	);
	// 描画（メッシュ）
	void DrawMesh(
		const Mesh* a_mesh,
		const DirectX::XMFLOAT4X4& a_worldMat,
		const DirectX::XMFLOAT4& a_colorScale = { 1,1,1,1 },
		const DirectX::XMFLOAT3& a_emissive = { 1,1,1 }
	);

public:
	// ゲッター
	ID3D12Device6* GetDevice();						// デバイス
	ID3D12GraphicsCommandList* GetCommandList();	// コマンドリスト
	UINT CurrentBackBufferIndex();					// 現在のフレーム番号

private:
	// DirectX12初期化に使う
	bool CreateSwapChain(HWND a_hWnd,UINT a_frameBufferWidth,UINT a_frameBufferHeight);		// スワップチェインの生成
	void CreateViewPort(UINT a_frameBufferWidth, UINT a_frameBufferHeight);		// ビューポートを生成
	void CreateScissorRect(UINT a_frameBufferWidth, UINT a_frameBufferHeight);	// シザー短径を生成（指定した範囲に描画を行う技術）

private:
	// 描画に使うDirectX12のオブジェクト
	UINT m_currentBackBufferIndex = 0;													// 現在のバッファ添え字

	Device m_device;																	// デバイス
	CommandQueue m_commandQueue;														// コマンドキュー
	ComPtr<IDXGISwapChain3> m_pSwapChain = nullptr;										// スワップチェイン
	CommandAllocator m_commandAllocator;												// コマンドアロケーター
	CommandList m_commandList;															// コマンドリスト
	HANDLE m_fenceEvent = nullptr;														// フェンスで使うイベント
	Fence m_fence;																		// フェンス
	UINT64 m_fenceValue[FRAME_BUFFER_COUNT];											// フェンスの数
	D3D12_VIEWPORT m_viewPort;															// ビューポート
	D3D12_RECT m_scissor;																// シザー矩形

private:
	// 描画に使うオブジェクトとその生成関数群
	bool CreateRenderTarget();			// レンダーターゲットを生成
	bool CreateDepthStencil(UINT a_frameBufferWidth, UINT a_frameBufferHeight);			// 深度ステンシルバッファを生成

	UINT m_rtvDescriptorSize = 0;						// レンダーターゲットビューのディスクリプタサイズ
	ComPtr<ID3D12DescriptorHeap> m_pRtvHeap = nullptr;	// レンダーターゲットのディスクリプタヒープ
	ComPtr<ID3D12Resource> m_pRenderTargets[FRAME_BUFFER_COUNT] = { nullptr };		// レンダーターゲット

	UINT m_dsvDescriptorSize = 0;								// 深度ステンシルのディスクリプターサイズ
	ComPtr<ID3D12DescriptorHeap> m_pDsvHeap = nullptr;			// 深度ステンシルのディスクリプタヒープ
	ComPtr<ID3D12Resource> m_pDeptchStencilBuffer = nullptr;	// 深度ステンシルバッファ（こっちは一つ）

	std::shared_ptr<RootSignature> m_spRootSignature = nullptr;		// ルートシグネチャ
	std::shared_ptr<PipelineState> m_spPipeLineState = nullptr;		// パイプラインステート

	// カメラ用定数バッファ
	std::shared_ptr<ConstantBuffer> m_spCameraConstantBuffer[FRAME_BUFFER_COUNT] = { nullptr };	// カメラ用定数バッファ
	std::shared_ptr<DescriptorHeap> m_spDescriptorHeap = nullptr;	// ディスクリプタヒープ

private:
	// 描画ループで使用するもの
	ID3D12Resource* m_currentRenderTarget = nullptr;		// 現在のフレームのレンダーターゲット
	void WaitRender();										// 描画完了を待つ処理

private:
	// シングルトン
	RenderingEngine(){}
	~RenderingEngine(){}
public:
	// インスタンス取得
	static RenderingEngine& Instance()
	{
		static RenderingEngine _instance;
		return _instance;
	}
};

