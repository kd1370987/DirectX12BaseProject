#pragma once

class RootSignature;
class RootSignatureManager;

class CBAllocater;

namespace Engine::Resource
{
	class ShaderManager;
	class QuadPolygon;
}

namespace Engine::D3D12
{
	class GraphicsPSOManager;
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
		Resource::Handle<SRV> srvHandleRange = {};

		DirectX::XMFLOAT4X4 worldMat = {};
		DirectX::XMFLOAT4	colorScale = { 1,1,1,1 };
	};

	struct DebugDrawInfo
	{
		UINT startIndex;		// インデックスバッファの開始位置
	};

	class ShapeRenderer;
	
	// 現在のフレームの描画管理クラス
	class RenderContext
	{
	public:

		// フレームで消費するリソース
		struct FrameResource
		{
			// カメラとオブジェクトの定数バッファアロケーター
			std::shared_ptr<CBAllocater> spCamAndObjectCBAllocater = nullptr;
			VertexBuffer shapeVertexBuffer;
		};


	public:

		//--------------------------------------------------------------------------------------------
		// クラス基盤
		//--------------------------------------------------------------------------------------------
		
		RenderContext();
		~RenderContext();

		// 初期化・解放
		void Init(
			Resource::ShaderManager* a_pShaderMana,
			RootSignatureManager* a_pRootSigMana,
			Engine::D3D12::GraphicsPSOManager* a_pPSOMana,
			ShapeRenderer* a_pShapeRender
		);
		void Shutdown();

		// フレームの初めと終わりの処理
		void BeginFrame();
		void EndFrame();

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

		//--------------------------------------------------------------------------------------------
		// バッファ関係
		//--------------------------------------------------------------------------------------------
		// 現在のフレームの定数バッファアロケーターにアクセス
		CBAllocater* BindCB();

		// レンダーターゲットの切り替え
		// 基本的にハンドルで管理しているため内部以外では直接触らない
		void ChangeRenderTarget(
			const std::vector<Resource::Handle<RTV>>& a_rtvHandleVec,
			const Resource::Handle<DSV>& a_dsvHandle
		);

		// SRVのバインド
		// 直接数字を指定してのバインド
		void BindSRV(
			int a_rootIndex,
			const std::vector<D3D12_GPU_DESCRIPTOR_HANDLE>& a_srvHandle
		);
		// 割り当てられているルート番号を探してのバインド
		void BindSRV(
			RootSigSemantic a_sema,
			const std::vector<D3D12_GPU_DESCRIPTOR_HANDLE>& a_srvHandle
		);

		// レンダーターゲットのクリア
		void ClearRenderTarget(const Resource::Handle<Resource::Texture>& a_texHandle);

		// 深度値バッファのクリア
		void ClearDSV(const Resource::Handle<DSV>& a_DSVHandle);

		// 矩形描画のためのクラス取得
		ShapeRenderer* RefShapeDraw();


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
		void SetGraphicsRootSignature(const Resource::ID& a_rootSigID);

		// パイプラインステートをセット、前回と変更がない場合はスキップ
		void SetGraphicPSO(const Resource::Handle<D3D12::PipelineState>& a_handle);

		// 1Draw当たりのオブジェクトに対する定数
		void BindObuje(
			const DirectX::XMFLOAT2& a_uv = { 0.0f,0.0f },
			const DirectX::XMFLOAT2& a_tile = { 1.0f,1.0f }
		);

		// マテリアルをSRVとして送信、その際にマテリアルの定数も送信
		void BindMaterial(
			const Resource::Material* a_pMaterial,
			const DirectX::XMFLOAT4& a_colorScale,
			const DirectX::XMFLOAT3& a_emissiveScale
		);

		// mesh情報を送信、前回と変更がなければスキップ
		void BindMesh(
			Resource::Mesh* a_pMesh,
			const DirectX::XMFLOAT4X4& a_worldMat
		);

		// ボーン行列を送信、配列と長さを入れる
		void BindBone(
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

		void DrawQueue(
			RenderPassID a_passID,
			LightingType a_lightingType
		);

		// UI描画
		void DrawUIQueue(RenderQueueType2D a_type);

		// レンダーグラフのテクスチャのハンドル取得
		D3D12_GPU_DESCRIPTOR_HANDLE GetImGuiGPUHandle(const std::string& a_name);

		// プリミティブトポロジーの設定
		void SetPrimitive(D3D_PRIMITIVE_TOPOLOGY a_topology);

		// ラスタライザーモード設定
		void SetRasterizerFillMode(D3D12_FILL_MODE a_fillMode);

		// 形状描画
		void ShapeDraw();

	private:

		// 1フレームで消費するリソース
		FrameResource m_frameResource[CPU_FRAME_COUNT] = {};

		// マネージャー
		Resource::ShaderManager*			m_pShaderManger			= nullptr;
		RootSignatureManager*				m_pRootSigManager		= nullptr;
		Engine::D3D12::GraphicsPSOManager*	m_pGraphicsPSOManager	= nullptr;

		// 形状描画クラス
		ShapeRenderer* m_pShapeDraw = nullptr;

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

		// ECSからの分離
		std::unordered_map<RenderQueueType, std::vector<DrawItem>> m_drawItemMap = {};

		std::unordered_map<RenderQueueType2D, std::vector<DrawItem2D>> m_drawItem2DMap = {};


		std::vector<DrawItem> m_drawItemVec = {};


		std::shared_ptr<Resource::QuadPolygon> m_spQuadPolygon = nullptr;
	};
}