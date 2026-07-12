#pragma once
#include "../CBData.h"
#include "Engine/D3D12/CBAllocater/CBAllocater.h"

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
	class ShapeRenderer;
	class GraphicsEngine;

	struct DrawItem2D
	{
		Handle<D3D12::SRV> srvHandleRange = {};

		DirectX::XMFLOAT4X4 worldMat = {};
		DirectX::XMFLOAT4	colorScale = { 1,1,1,1 };
	};

	struct DebugDrawInfo
	{
		UINT startIndex;		// インデックスバッファの開始位置
	};

	

	// レンダーコンテキスト作成時に必要な情報
	struct RenderContextDesc
	{
		// D3Dオブジェクトのキャッシュ
		D3D12::Device* pDevice = nullptr;

		// クラスのキャッシュ
		ShapeRenderer*					pShapeRender	= nullptr;

		// アロケーターのメモリ容量
		size_t cbAllocatorMemSize = 32 * 1024 * 1024;

		// ボーン用行列数
		UINT boneElementNum = 0;
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

		ID3D12DescriptorHeap* GetCBV_SRV_UAVHeap() const;

		// 現在のコマンドリストを取得
		D3D12::GraphicsCommandList* GetCurrentCmdList();

		void SetDirectCommandList(D3D12::GraphicsCommandList* a_pCmdList);

		//--------------------------------------------------------------------------------------------
		// バッファ関係
		//--------------------------------------------------------------------------------------------
		// 現在のフレームの定数バッファアロケーターにアクセス
		CBAllocater* BindCB();

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
		// 基本的にハンドルで管理しているため内部以外では直接触らない
		void ChangeRenderTarget(
			const std::vector<Handle<D3D12::RTV>>& a_rtvHandleVec,
			const Handle<D3D12::DSV>& a_dsvHandle
		);
		void SetRenderTargets(
			const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& a_rtvHandleVec,
			const D3D12_CPU_DESCRIPTOR_HANDLE* a_pDsvHandle
		);

		// テクスチャハンドルからSRVをバインドする
		void BindSRV(UINT a_rootIdx, std::vector<Handle<Resource::Texture>>& a_texHandles);

		// SRVハンドルをもらってコピーする
		void BindSRV(UINT a_rootIdx, std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& a_cpuHandles);
		void BindSRV(UINT a_rootIdx, D3D12_CPU_DESCRIPTOR_HANDLE& a_cpuHandle);
		void BindSRV(UINT a_rootIdx,Handle<D3D12::SRV> a_srvHandle);

		void ComputeBindSRV(UINT a_rootIdx, D3D12_CPU_DESCRIPTOR_HANDLE a_cpuHandle);
		void ComputeBindSRV(UINT a_rootIdx, std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& a_cpuHandles);
		void ComputeBindSRV(UINT a_rootIdx, Handle<D3D12::SRV> a_srvHandle);

		void ComputeBindSRVBindLess(UINT a_rootIdx, Handle<D3D12::SRV> a_srvHandle);
		

		// UAV
		void BindUAV(UINT a_rootIdx, D3D12_CPU_DESCRIPTOR_HANDLE a_cpuHandle);
		void BindUAV(UINT a_rootIdx, Handle<D3D12::UAV> a_uavHandle);
		void BindUAV(UINT a_rootIdx, std::vector<Handle<D3D12::UAV>> a_uavHandles);
		void BindUAVBindLess(UINT a_rootIdx, Handle<D3D12::UAV> a_handle);

		// 直接GPUアドレスを取得
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE a_cpuHandle);
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> a_cpuHandles);
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandleBindLess(Handle<D3D12::SRV> a_handle);

		// レンダーターゲットのクリア
		void ClearRenderTarget(const Handle<Resource::Texture>& a_texHandle);
		void ClearRenderTarget(const D3D12_CPU_DESCRIPTOR_HANDLE& a_rtvHandle);

		// 深度値バッファのクリア
		void ClearDSV( const Handle<D3D12::DSV>& a_DSVHandle);
		void ClearDSV( const D3D12_CPU_DESCRIPTOR_HANDLE& a_DSVHandle);

		// 矩形描画のためのクラス取得
		ShapeRenderer* RefShapeDraw();

		// ヒープのセット
		void BindHeap();
		void BindCopyHeapAndSumpler();
		void BindCopyHeapAndSumplerBindLess();
		void BindHeaps(UINT a_numHeaps, ID3D12DescriptorHeap *const* a_pHeaps);

		// バインドボーン
		// すべてのアニメーション行列を配置しているから一括で送れる
		// 定数バッファでスタートインデックスとカウントを送る必要あり
		void BindSRVBone();

		void Dispatch(UINT a_x,UINT a_y,UINT a_z);
		void DispatchMesh(UINT a_x,UINT a_y,UINT a_z);

		// カメラのバインド
		void BindGraphicsCamera();

		//--------------------------------------------------------------------------------------------
		// 描画コマンド
		//--------------------------------------------------------------------------------------------


		// 描画命令の実行
		void UpdateBuffer(
			const std::vector<InstanceData>& a_instanceVec,
			const std::vector<SubSetData>& a_subsetVec,
			const std::vector<MeshInstanceData>& a_mesInstance,
			const std::vector<MeshMaterial>& a_mesMaterial
		);

		// インデックスバインド
		void BindIndex(UINT a_instanceBufferIndex, UINT a_subsetBufferIndex, UINT a_rootIndex = 1);

		// バッファバインド
		void BindInstanceBuffer(UINT a_rootIndex);
		void BindSubsetBuffer(UINT a_rootIndex);
		void BindBonePalletBuffer(UINT a_rootIndex);
		void ComputeBindBonePalletBuffer(UINT a_rootIndex);
		void BindGraphicsDebugLineBuffer(UINT a_rootIndex);

		// メッシュシェーダー関連
		void BindCamera();
		void BindMeshInstance();
		void BindMeshlet();

		void DrawQueueDispathMesh(uint8_t a_passIndex);

		//--------------------------------------------------------------------------------------------
		// 描画パス構築
		//--------------------------------------------------------------------------------------------
		
		// テクスチャのコピー
		void TexCopy(const Handle<Resource::Texture>& a_src,const Handle<Resource::Texture>& a_dst);
		void ResourceCopy(ID3D12Resource* a_pSrc,ID3D12Resource* a_pDst);

		// グラフィックスルートシグネチャをセット、前回と変更がない場合はスキップ
		void SetGraphicsRootSignature(ID3D12RootSignature* a_pRootSig);
		void SetComputeRootSignature(ID3D12RootSignature* a_pRootSig);

		// パイプラインステートをセット、前回と変更がない場合はスキップ
		void SetGraphicPSO(ID3D12PipelineState* a_pPSO);
		void SetGraphicPSO(uint8_t a_pPsoIndex);
		void SetComputePSO(ID3D12PipelineState* a_pPSO);
		void SetComputePSO(uint8_t a_pPsoIndex);

		// プリミティブトポロジーセット
		void SetPrimitive(D3D12_PRIMITIVE_TOPOLOGY a_pri);

	
		void BindMaterialSRV(
			UINT a_index,
			const Resource::Material* a_pMaterial
		);
		void BindMaterialSRV(
			UINT a_index,
			uint16_t a_materialID
		);

		void BindMesh(uint16_t a_meshID);
	

		// モデルの描画
		void Draw(const Resource::Mesh* a_pMesh,UINT a_subIdx);
		void Draw(uint16_t a_meshID,UINT a_subIdx);

		// パーティクルやUIなどの描画用
		void DrawPolygonInstancing(UINT a_count);
		// クワッド描画
		void DrawQuad();

		// 形状描画用
		void DrawShape();


		//リソースバリア設定
		void Transition(
			ID3D12Resource* a_pResource,
			D3D12_RESOURCE_STATES a_before,
			D3D12_RESOURCE_STATES a_after
		);
		void UAVBarrier(ID3D12Resource* a_pResource);

		// バックバッファに切り替え
		void ChangeBackBuffer();

	private:
		//--------------------------------------------------------------------------------------------
		// 参照
		//--------------------------------------------------------------------------------------------
		D3D12::Device* m_pDevice = nullptr;						// デバイス
		GraphicsEngine* m_pGraphicsEngine = nullptr;			// オーナー
		ShapeRenderer* m_pShapeDraw = nullptr;					// 形状描画クラス

		//--------------------------------------------------------------------------------------------
		// フレーム限定リソース
		//--------------------------------------------------------------------------------------------
		std::unique_ptr<CBAllocater> m_upCBAllocater = nullptr;	// 定数バッファアロケーター
		D3D12::GraphicsCommandList* m_pCmdList = nullptr;				// 現在フレームのグラフィックスコマンドリスト

		// コピー用ヒープ
		D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>	m_copyHeap;		// ラスタライザ用
		D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>	m_bindLessHeap;	// バインドレス用
		UINT m_currentHeapOffset = 0;

		// オブジェクト単位データ
		D3D12::StaticStructuredBuffer<InstanceData> m_instanceBuffer;
		// サブメッシュ単位データ
		D3D12::StaticStructuredBuffer<SubSetData> m_subsetBuffer;

		// ボーン用データ
		D3D12::DynamicStructuredBuffer<Resource::BoneMatrix> m_boneBuffer;

		// アニメーション用頂点データ
		D3D12::RWStructuredBuffer<Resource::MeshVertexFloat> m_skininedVerticesBuffer;

		// デバッグライン用頂点
		D3D12::StaticStructuredBuffer<DebugLineData> m_debugLineBuffer;

		// 描画用ポリゴン
		std::shared_ptr<Resource::QuadPolygon> m_spQuadPolygon = nullptr;

		// メッシュシェーダー用データ
		D3D12::StaticStructuredBuffer<MeshInstanceData>		m_meshInstanceBuffer;
		D3D12::StaticStructuredBuffer<MeshMaterial>			m_meshMaterialBuffer;
	};
	template<typename T>
	inline void RenderContext::GraphicsBindRootCBV(int a_descIndex, const T& a_data)
	{
		m_upCBAllocater->BindAndAttachDataRootCBV(m_pCmdList, a_descIndex, a_data);
	}
	template<typename T>
	inline void RenderContext::ComputeBindRootCBV(int a_descIndex, const T & a_data)
	{
		m_upCBAllocater->BindAndAttachDataComputeRootCBV(m_pCmdList, a_descIndex, a_data);
	}
}