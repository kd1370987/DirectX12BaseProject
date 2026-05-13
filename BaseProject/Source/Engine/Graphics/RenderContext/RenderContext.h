#pragma once


class CBAllocater;

namespace Engine::Resource
{
	class QuadPolygon;
}

namespace Engine::D3D12
{
	class GraphicsPSOManager;
	class CommandList;
	class RootSignature;
	class RootSignatureManager;
}

namespace Engine::Graphics
{
	class RenderGraph;

	enum class RenderPassID
	{
		ZPrePass,
		ShadowMapPass,
		GBufferPass,
		ForwardPass,
		PostProcessPass,
		ScreenUIPass,
	};

	enum class LightingType
	{
		None,
		Tone,
		PBR,
	};

	// １DrawCall当たりアイテム
	struct DrawItem
	{
		RenderPassID passID = RenderPassID::ZPrePass;
		LightingType lightingType = LightingType::PBR;

		const Resource::Material* pMaterial;
		UINT subIdx = 0;
		Resource::Mesh* pMesh = nullptr;

		const DirectX::XMFLOAT4X4* pBoneMatrices = nullptr;
		UINT boneCount = 0;

		DirectX::XMFLOAT4X4 worldMat = {};
		DirectX::XMFLOAT4	colorScale = { 1,1,1,1 };
		DirectX::XMFLOAT3	emissiveScale = { 1,1,1 };
	};

	struct DrawItem2D
	{
		Resource::Handle<D3D12::SRV> srvHandleRange = {};

		DirectX::XMFLOAT4X4 worldMat = {};
		DirectX::XMFLOAT4	colorScale = { 1,1,1,1 };
	};

	struct DebugDrawInfo
	{
		UINT startIndex;		// インデックスバッファの開始位置
	};

	class ShapeRenderer;

	// レンダーコンテキスト作成時に必要な情報
	struct RenderContextDesc
	{
		// D3Dオブジェクトのキャッシュ
		ID3D12Device* pDevice = nullptr;

		// クラスのキャッシュ
		D3D12::RootSignatureManager*	pRootSigMana	= nullptr;
		D3D12::GraphicsPSOManager*		pPSOMana		= nullptr;
		ShapeRenderer*					pShapeRender	= nullptr;

		// アロケーターのメモリ容量
		size_t cbAllocatorMemSize = 32 * 1024 * 1024;
	};

	// マイフレームリセットするときに外部からもらう情報
	struct FrameDesc
	{
		ID3D12GraphicsCommandList* pCmdList = nullptr;
		D3D12::CommandList* pCmdListClass = nullptr;
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
		void Init(const RenderContextDesc& a_desc);

		// フレームの初めに呼ぶ
		void Begine(const FrameDesc& a_desc);
		void Clear();

		ID3D12DescriptorHeap* GetCBV_SRV_UAVHeap() const;

		// 現在のコマンドリストを取得
		D3D12::CommandList* GetCurrentCmdList();

		//--------------------------------------------------------------------------------------------
		// カメラ関係
		//--------------------------------------------------------------------------------------------
		// 描画時情報セット
		void SetToShader(const DirectX::XMFLOAT4X4& a_worldMat);		// ワールド行列
		void SetProjectionMatrix(DirectX::XMFLOAT4X4 a_projMat);		// プロジェクション行列

		// 現在の描画しているカメラの情報を取得
		float GetCameraAspectRate();									// アスペクトレート
		const DirectX::XMFLOAT4X4& GetCameraRotMat();					// 回転行列
		const DXSM::Vector3& GetCameraPOS();							// 座標

		// 描画直前にカメラの情報をGPU側に送る
		void BindCameraCB();
		void BindAmbientCB();

		//--------------------------------------------------------------------------------------------
		// バッファ関係
		//--------------------------------------------------------------------------------------------
		// 現在のフレームの定数バッファアロケーターにアクセス
		CBAllocater* BindCB();

		// 定数バッファをルートでバインド
		void BindRootCBV(UINT a_index,const void* a_pData,size_t a_size);
		template<typename T>
		void BindRootCBV(UINT a_index,const T& a_data);

		// レンダーターゲットの切り替え
		// 基本的にハンドルで管理しているため内部以外では直接触らない
		void ChangeRenderTarget(
			const std::vector<Resource::Handle<D3D12::RTV>>& a_rtvHandleVec,
			const Resource::Handle<D3D12::DSV>& a_dsvHandle
		);

		// テクスチャハンドルからSRVをバインドする
		void BindSRV(UINT a_rootIdx, std::vector<Resource::Handle<Resource::Texture>>& a_texHandles);

		// SRVハンドルをもらってコピーする
		void BindSRV(UINT a_rootIdx, std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& a_cpuHandles);
		void BindSRV(UINT a_rootIdx, D3D12_CPU_DESCRIPTOR_HANDLE& a_cpuHandle);

		// UAV
		void BindUAV(UINT a_rootIdx, D3D12_CPU_DESCRIPTOR_HANDLE a_cpuHandle);

		// 直接GPUアドレスを取得
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE a_cpuHandle);
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> a_cpuHandles);

		// レンダーターゲットのクリア
		void ClearRenderTarget(const Resource::Handle<Resource::Texture>& a_texHandle);

		// 深度値バッファのクリア
		void ClearDSV(const Resource::Handle<D3D12::DSV>& a_DSVHandle);

		// 矩形描画のためのクラス取得
		ShapeRenderer* RefShapeDraw();

		// ヒープのセット
		void BindHeap();
		void BindHeaps(UINT a_numHeaps, ID3D12DescriptorHeap *const* a_pHeaps);

		//--------------------------------------------------------------------------------------------
		// 描画コマンド
		//--------------------------------------------------------------------------------------------
		// 描画命令の追加
		void AddItem(const RenderQueueType& a_type, const DrawItem& a_item);		// 3D
		void AddItem(RenderQueueType2D a_type, const DrawItem2D& a_itemVec);		// 2D

		// 描画命令の取得
		const std::vector<DrawItem>& GetItemVec(const RenderQueueType& a_type) const;		// 3D
		const std::vector<DrawItem2D>& GetItemVec(const RenderQueueType2D& a_type) const;	// 2D

		// 描画命令の実行
		void Excute(RenderGraph* a_pGraph);

		// 描画命令のコマンドリスト削除
		void ClearCmd();

		//--------------------------------------------------------------------------------------------
		// 描画パス構築
		//--------------------------------------------------------------------------------------------
		// グラフィックスルートシグネチャをセット、前回と変更がない場合はスキップ
		void SetGraphicsRootSignature(ID3D12RootSignature* a_pRootSig);

		// パイプラインステートをセット、前回と変更がない場合はスキップ
		void SetGraphicPSO(ID3D12PipelineState* a_pPSO);

		// プリミティブトポロジーセット
		void SetPrimitive(D3D12_PRIMITIVE_TOPOLOGY a_pri);

		// 1Draw当たりのオブジェクトに対する定数
		void BindObuje(
			UINT a_index,
			const DirectX::XMFLOAT2& a_uv = { 0.0f,0.0f },
			const DirectX::XMFLOAT2& a_tile = { 1.0f,1.0f }
		);

		// マテリアルをSRVとして送信、その際にマテリアルの定数も送信
		void BindMaterial(
			UINT a_index,
			const Resource::Material* a_pMaterial,
			const DirectX::XMFLOAT4& a_colorScale,
			const DirectX::XMFLOAT3& a_emissiveScale
		);
		void BindMaterialSRV(
			UINT a_index,
			const Resource::Material* a_pMaterial
		);
		// ルートパラメタインデックスを指定してのメッシュバインド
		void BindMesh(
			UINT a_index,
			Resource::Mesh* a_pMesh,
			const DirectX::XMFLOAT4X4& a_worldMat

		);
		// ボーン行列を送信、配列と長さを入れる
		void BindBone(
			UINT a_index,
			const DirectX::XMFLOAT4X4* a_pMatVec,
			UINT a_count
		);

		// ビューポート設定
		void SetViewPort();

		// シザーレクト設定
		void SetScissorRect();

		// モデルの描画
		void Draw(
			Resource::Mesh* a_pMesh,
			UINT a_subIdx
		);


		//リソースバリア設定
		void Transition(
			ID3D12Resource* a_pResource,
			D3D12_RESOURCE_STATES a_before,
			D3D12_RESOURCE_STATES a_after
		);

		// バックバッファに切り替え
		void ChangeBackBuffer();


		// クワッド描画
		void DrawQuad();

		// UI描画
		void DrawUIQueue(RenderQueueType2D a_type);

		// 形状描画
		void ShapeDraw();

	private:
		// D3DObject
		ID3D12Device* m_pDevice = nullptr;

		// マネージャー
		D3D12::RootSignatureManager*				m_pRootSigManager		= nullptr;
		Engine::D3D12::GraphicsPSOManager*	m_pGraphicsPSOManager	= nullptr;

		// 形状描画クラス
		ShapeRenderer* m_pShapeDraw = nullptr;
		//VertexBuffer m_shapeVertexBuffer = {};
		D3D12::DynamicVertexBuffer<Resource::Vertex> m_shapeVertexBuffer = {};
		

		//--------------------------------------------------------------------------------------------
		// フレーム限定リソース
		//--------------------------------------------------------------------------------------------
		//ID3D12GraphicsCommandList* m_pCmdList = nullptr;		// フレームごとにもらい受ける
		std::unique_ptr<CBAllocater> m_upCBAllocater = nullptr;	// 定数バッファアロケーター
		D3D12::CommandList* m_pCmdList = nullptr;
		// コピー用ヒープ
		D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV> m_copyHeap = {};
		UINT m_currentHeapOffset = 0;



		// 定数バッファ
		CBCamera m_cb0_camera = {};
		float m_aspectRate = 0.0f;

		CBObject m_cb1_object = {};
		CBMeshTrans m_cb2_MeshTrans = {};
		CBMaterial m_cb3_Material = {};
		CBBone m_cb4_Bone = {};
		CBAmbient m_cb5_Ambient = {};
		CBUI m_cbUI = {};

		// 描画コマンド
		Resource::ID m_currentRootSigID = Resource::Limits::INVALID_ID;
		Resource::Handle<D3D12::PipelineState> m_currentPSOID;
		const Resource::Material* m_pCurrentMaterial = nullptr;
		Resource::Mesh* m_pCurrentMesh = nullptr;
		Resource::QuadPolygon* m_pCurrentPoly = nullptr;

		// 3D用描画アイテムキュー
		std::unordered_map<RenderQueueType, std::vector<DrawItem>> m_drawItemMap = {};

		// 2D用描画アイテムキュー
		std::unordered_map<RenderQueueType2D, std::vector<DrawItem2D>> m_drawItem2DMap = {};

		// 描画用ポリゴン
		std::shared_ptr<Resource::QuadPolygon> m_spQuadPolygon = nullptr;
	};
	template<typename T>
	inline void RenderContext::BindRootCBV(UINT a_index, const T& a_data)
	{
		BindRootCBV(a_index, a_data, sizeof(T));
	}
}