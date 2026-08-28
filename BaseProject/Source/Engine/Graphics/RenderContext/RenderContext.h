#pragma once
#include "../CBData.h"
#include "Engine/D3D12/CBAllocator/CBAllocator.h"

namespace Engine::Resource
{
	class QuadPolygon;
}

namespace Engine::D3D12
{
	class RootSignature;
}


namespace Engine::Graphics
{
	// 前方宣言
	class RenderGraph;
	class GraphicsEngine;

	// レンダーコンテキスト作成時に必要な情報
	struct RenderContextDesc
	{
		// D3Dオブジェクトのキャッシュ
		D3D12::Device* pDevice = nullptr;

		// アロケーターのメモリ容量
		size_t cbAllocatorMemSize = 32 * 1024 * 1024;

		// ボーン用行列数
		UINT boneElementNum = 0;	// ボーンパレットの要素数(重ねたシーンぶんを連結するので、1ワールド分では足りない)
	};
	


	// 現在のフレームの描画管理クラス
	class RenderContext
	{
	public:

		//--------------------------------------------------------------------------------------------
		// クラス基盤
		//--------------------------------------------------------------------------------------------
		RenderContext();
		~RenderContext();

		// 初期化・解放
		void Init(
			GraphicsEngine* a_pOwner,
			D3D12::GraphicsCommandList* a_pCmdList,
			const RenderContextDesc& a_desc
		);
		void Release();

		// フレームの初めに呼ぶ
		void Clear();

		// 現在のコマンドリストを取得
		D3D12::GraphicsCommandList* GetCurrentCmdList();

		void SetDirectCommandList(D3D12::GraphicsCommandList* a_pCmdList);

		//--------------------------------------------------------------------------------------------
		// バッファ関係
		//--------------------------------------------------------------------------------------------
		// 現在のフレームの定数バッファアロケーターにアクセス
		CBAllocator* BindCB();

		// ---- 定数バッファをルートでバインド ----
		// グラフィック版
		template<typename T>
		void GraphicsBindRootCBV(
			int a_descIndex,
			const T& a_data
		);
		// コンピュート版
		template<typename T>
		void ComputeBindRootCBV(
			int a_descIndex,
			const T& a_data
		);

		// レンダーターゲットの切り替え
		void SetRenderTargets(
			const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& a_rtvHandleVec,
			const D3D12_CPU_DESCRIPTOR_HANDLE* a_pDsvHandle
		);

		// SRVのバインド(現在キャッシュしているヒープへコピーしてディスクリプタテーブルを張る)
		// テクスチャハンドル配列から
		void BindSRV(UINT a_rootIdx, std::vector<Handle<Resource::Texture>>& a_texHandles);
		// レンダーグラフがコンパイル時に焼き込んだ連続領域をそのまま渡せるようspanで受ける
		void BindSRV(UINT a_rootIdx, std::span<const D3D12_CPU_DESCRIPTOR_HANDLE> a_cpuHandles);
		void BindSRV(UINT a_rootIdx, D3D12_CPU_DESCRIPTOR_HANDLE a_cpuHandle);
		void BindSRV(UINT a_rootIdx,Handle<D3D12::SRV> a_srvHandle);

		void ComputeBindSRV(UINT a_rootIdx, D3D12_CPU_DESCRIPTOR_HANDLE a_cpuHandle);
		void ComputeBindSRV(UINT a_rootIdx, std::span<const D3D12_CPU_DESCRIPTOR_HANDLE> a_cpuHandles);
		void ComputeBindSRV(UINT a_rootIdx, Handle<D3D12::SRV> a_srvHandle);

		void ComputeBindSRVBindLess(UINT a_rootIdx, Handle<D3D12::SRV> a_srvHandle);


		// UAV
		void BindUAV(UINT a_rootIdx, D3D12_CPU_DESCRIPTOR_HANDLE a_cpuHandle);
		void BindUAV(UINT a_rootIdx, Handle<D3D12::UAV> a_uavHandle);
		void BindUAV(UINT a_rootIdx, std::vector<Handle<D3D12::UAV>> a_uavHandles);
		void BindUAVBindLess(UINT a_rootIdx, Handle<D3D12::UAV> a_handle);

		// 直接GPUアドレスを取得
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> a_cpuHandles);
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandleBindLess(Handle<D3D12::SRV> a_handle);

		// レンダーターゲットのクリア
		void ClearRenderTarget(const Handle<Resource::Texture>& a_texHandle);
		void ClearRenderTarget(const D3D12_CPU_DESCRIPTOR_HANDLE& a_rtvHandle);

		// 深度値バッファのクリア
		void ClearDSV( const Handle<D3D12::DSV>& a_DSVHandle);
		void ClearDSV( const D3D12_CPU_DESCRIPTOR_HANDLE& a_DSVHandle);

		// ヒープのセット(セットしたヒープを m_pCurrentHeap にキャッシュする)
		void BindHeap();
		void BindCopyHeapAndSumplerBindLess();

		void Dispatch(UINT a_x,UINT a_y,UINT a_z);
		void DispatchMesh(UINT a_x,UINT a_y,UINT a_z);

		//--------------------------------------------------------------------------------------------
		// 描画コマンド
		//--------------------------------------------------------------------------------------------


		// 描画命令の実行
		// a_boneMatVec : このフレームに描くワールドすべてのボーン行列を連結したもの。
		// シーンから直接引かず渡してもらう(重ねて描くときに描く側しか全体を知らないため)
		void UpdateBuffer(
			const std::vector<MeshInstanceData>& a_mesInstance,
			const std::vector<MeshMaterial>& a_mesMaterial,
			const std::vector<Resource::BoneMatrix>& a_boneMatVec
		);
		void UpdateUIBuffer(const std::vector<UIData>& a_uiInstanceVec);

		// バッファバインド
		void ComputeBindBonePalletBuffer(UINT a_rootIndex);
		void BindGraphicsDebugLineBuffer(UINT a_rootIndex);

		// カメラ・メッシュシェーダー関連
		void BindCamera();
		void BindMeshInstance();
		void BindMeshlet();

		// UI関連
		void BindUIBuffer(UINT a_rootIndex);
		void DrawUI();

		void DrawQueueDispathMesh(uint8_t a_passIndex);

		//--------------------------------------------------------------------------------------------
		// 描画パス構築
		//--------------------------------------------------------------------------------------------

		// リソースのコピー
		void ResourceCopy(ID3D12Resource* a_pSrc,ID3D12Resource* a_pDst);

		// ルートシグネチャをセット
		void SetGraphicsRootSignature(ID3D12RootSignature* a_pRootSig);
		void SetComputeRootSignature(ID3D12RootSignature* a_pRootSig);

		// ハンドル版 : PipelineStateManager から実体を引いて張る。
		// 保持側はハンドルのまま持ち、張る直前にここで解決する
		void SetGraphicsRootSignature(const Handle<ID3D12RootSignature>& a_handle);
		void SetComputeRootSignature(const Handle<ID3D12RootSignature>& a_handle);

		// パイプラインステートをセット、前回と変更がない場合はスキップ
		void SetGraphicPSO(ID3D12PipelineState* a_pPSO);
		void SetGraphicPSO(uint8_t a_pPsoIndex);
		void SetComputePSO(ID3D12PipelineState* a_pPSO);
		void SetComputePSO(uint8_t a_pPsoIndex);

		// プリミティブトポロジーセット
		void SetPrimitive(D3D12_PRIMITIVE_TOPOLOGY a_pri);

		// パーティクルやUIなどの描画用
		void DrawPolygonInstancing(UINT a_count);

		// 形状描画用
		void DrawShape();


		//リソースバリア設定
		void Transition(
			ID3D12Resource* a_pResource,
			D3D12_RESOURCE_STATES a_before,
			D3D12_RESOURCE_STATES a_after
		);
		
		// バックバッファに切り替え
		void ChangeBackBuffer();

	private:
		//--------------------------------------------------------------------------------------------
		// ディスクリプタコピーの共通処理
		//--------------------------------------------------------------------------------------------
		// 現在キャッシュしているヒープ(m_pCurrentHeap)へCPUハンドル群をコピーし、
		// 先頭のGPUハンドルを返す。容量オーバー時は ptr==0 のハンドルを返す。
		D3D12_GPU_DESCRIPTOR_HANDLE CopyToCurrentHeap(std::span<const D3D12_CPU_DESCRIPTOR_HANDLE> a_cpuHandles);
		// コピー後にグラフィック/コンピュートのディスクリプタテーブルを張る
		void GraphicsBindTable(UINT a_rootIdx, std::span<const D3D12_CPU_DESCRIPTOR_HANDLE> a_cpuHandles);
		void ComputeBindTable(UINT a_rootIdx, std::span<const D3D12_CPU_DESCRIPTOR_HANDLE> a_cpuHandles);

	private:
		//--------------------------------------------------------------------------------------------
		// 参照
		//--------------------------------------------------------------------------------------------
		D3D12::Device* m_pDevice = nullptr;						// デバイス
		GraphicsEngine* m_pGraphicsEngine = nullptr;			// オーナー

		//--------------------------------------------------------------------------------------------
		// フレーム限定リソース
		//--------------------------------------------------------------------------------------------
		std::unique_ptr<CBAllocator> m_upCBAllocator = nullptr;	// 定数バッファアロケーター
		D3D12::GraphicsCommandList* m_pCmdList = nullptr;				// 現在フレームのグラフィックスコマンドリスト

		// コピー用ヒープ
		D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>	m_copyHeap;		// ラスタライザ用
		D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>	m_bindLessHeap;	// バインドレス用
		UINT m_currentHeapOffset = 0;

		// 現在セットしているCBV_SRV_UAVヒープのキャッシュ(&m_copyHeap か &m_bindLessHeap)。
		// ヒープセット関数で更新し、SRV/UAVのバインドはこのヒープに対して行う。
		D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>*	m_pCurrentHeap = nullptr;

		// ボーン用データ
		D3D12::DynamicStructuredBuffer<Resource::BoneMatrix> m_boneBuffer;

		// デバッグライン用頂点
		D3D12::StaticStructuredBuffer<DebugLineData> m_debugLineBuffer;

		// 描画用ポリゴン
		std::shared_ptr<Resource::QuadPolygon> m_spQuadPolygon = nullptr;

		// メッシュシェーダー用データ
		D3D12::StaticStructuredBuffer<MeshInstanceData>		m_meshInstanceBuffer;
		D3D12::StaticStructuredBuffer<MeshMaterial>			m_meshMaterialBuffer;

		// UIデータ
		D3D12::DynamicStructuredBuffer<UIData> m_uiInstanceBuffer;
	};
	template<typename T>
	inline void RenderContext::GraphicsBindRootCBV(int a_descIndex, const T& a_data)
	{
		m_upCBAllocator->BindAndAttachDataRootCBV(m_pCmdList, a_descIndex, a_data);
	}
	template<typename T>
	inline void RenderContext::ComputeBindRootCBV(int a_descIndex, const T & a_data)
	{
		m_upCBAllocator->BindAndAttachDataComputeRootCBV(m_pCmdList, a_descIndex, a_data);
	}
}