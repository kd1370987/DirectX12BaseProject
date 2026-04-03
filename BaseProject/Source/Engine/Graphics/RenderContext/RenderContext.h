#pragma once

class RootSignature;

class DescriptorHeap;

class Mesh;
struct Material;

class ConstantBuffer;

class CBAllocater;

class ShaderManager;
class RootSignatureManager;
namespace Engine::D3D12
{
	class GraphicsPSOManager;
}

struct Model;
class QuadPolygon;


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

		Resource::Material* pMaterial = nullptr;
		UINT subIdx = 0;
		Resource::Mesh* pMesh = nullptr;

		DirectX::XMFLOAT4X4* pBoneMatrices = nullptr;
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

		/// <summary>
		/// 初期化・パイプラインステートやルートシグネチャの構築
		/// </summary>
		void Init();

		/// <summary>
		/// 終了処理
		/// </summary>
		void Shutdown();

		void BeginFrame();
		void EndFrame();

		/// <summary>
		/// カメラの情報を定数バッファに乗せてシェーダーに転送
		/// </summary>
		void SetToShader(
			const DirectX::XMFLOAT4X4& a_worldMat
		);

		// カメラ情報取得
		float GetCameraAspectRate()
		{
			return m_aspectRate;
		}
		const DirectX::XMFLOAT4X4& GetCameraRotMat();

		const DXSM::Vector3& GetCameraPOS();

		void BindCameraCB();

		void BindCB(RootSigSemantic a_sema);

		/// <summary>
		/// プロジェクション行列の設定
		/// </summary>
		/// <param name="a_fov">視野角</param>
		/// <param name="a_aspect">アスペクト比</param>
		/// <param name="a_near">ニアクリップ</param>
		/// <param name="a_far">ファークリップ</param>
		void SetProjectionMatrix(
			float a_fov,
			float a_aspect,
			float a_near,
			float a_far
		);

		/// <summary>
		/// プロジェクション行列の設定
		/// </summary>
		/// <param name="a_projMat">入れたい値</param>
		void SetProjectionMatrix(
			DirectX::XMFLOAT4X4 a_projMat
		);

		CBAllocater* BindCB();

		/// <summary>
		/// レンダーターゲットの切り替え
		/// </summary>
		/// <param name="a_cpuHnadleVec">RTVハンドル配列</param>
		/// <param name="a_depthHandle">深度値を使うのならDSVハンドル</param>
		void ChangeRenderTarget(
			const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& a_cpuHnadleVec,
			D3D12_CPU_DESCRIPTOR_HANDLE* a_depthHandle = nullptr
		);


		void BindSRV(
			RootSigSemantic a_sema,
			const std::vector<D3D12_GPU_DESCRIPTOR_HANDLE>& a_srvHandle
		);

		void ClearRenderTarget(
			const D3D12_CPU_DESCRIPTOR_HANDLE& a_renderTargetView,
			const DirectX::XMFLOAT4& a_colorRGBA = { 0.0f,0.0f,1.0f,1.0f },
			const UINT& a_numRects = 0,
			const D3D12_RECT* a_pRects = nullptr
		);

		void ClearDepth(const D3D12_CPU_DESCRIPTOR_HANDLE& a_depthHandle);

		ShapeRenderer* RefShapeDraw();

		//============================================================================================
		// 
		// 描画コマンド
		// 
		//============================================================================================

		void AddItem(const RenderQueueType& a_type, const DrawItem& a_item);

		void AddItem(RenderQueueType2D a_type, const DrawItem2D& a_itemVec);

		const std::vector<DrawItem>& GetItemVec(const RenderQueueType& a_type) const;

		const std::vector<DrawItem2D>& GetItemVec(const RenderQueueType2D& a_type) const;

		void Excute();

		void AddItem(const DrawItem& a_item);

		//============================================================================================
		// 
		// 描画パス構築
		// 
		//============================================================================================
		// グラフィックスルートシグネチャをセット、前回と変更がない場合はスキップ
		void SetGraphicsRootSignature(
			const Resource::ID& a_rootSigID
		);

		// パイプラインステートをセット、前回と変更がない場合はスキップ
		void SetGraphicPSO(const Resource::ID& a_psoID);
		void SetGraphicPSO(const Resource::Handle<D3D12::PipelineState>& a_handle);

		// 1Draw当たりのオブジェクトに対する定数
		void BindObuje(
			const DirectX::XMFLOAT2& a_uv = { 0.0f,0.0f },
			const DirectX::XMFLOAT2& a_tile = { 1.0f,1.0f }
		);

		// マテリアルをSRVとして送信、その際にマテリアルの定数も送信
		void BindMaterial(
			Resource::Material* a_pMaterial,
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
		void DrawUIQueue(
			RenderQueueType2D a_type
		);

		// レンダーグラフのテクスチャのハンドル取得
		D3D12_GPU_DESCRIPTOR_HANDLE GetImGuiGPUHandle(
			const std::string& a_name
		);

		// レンダーグラフが持っているリソースを返す
		std::vector<std::string> GetRGResourceList();

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
		std::shared_ptr<ShaderManager>			m_spShaderManger = nullptr;
		std::shared_ptr<RootSignatureManager>	m_spRootSigManager = nullptr;
		std::shared_ptr<D3D12::GraphicsPSOManager>		m_spGraphicsPSOManager = nullptr;

		// 形状描画クラス
		std::unique_ptr<ShapeRenderer> m_upShapeDraw = nullptr;

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
		Resource::Material* m_pCurrentMaterial = nullptr;
		Resource::Mesh* m_pCurrentMesh = nullptr;
		QuadPolygon* m_pCurrentPoly = nullptr;

		// ECSからの分離
		std::unordered_map<RenderQueueType, std::vector<DrawItem>> m_drawItemMap = {};

		std::unordered_map<RenderQueueType2D, std::vector<DrawItem2D>> m_drawItem2DMap = {};

		std::unique_ptr<RenderGraph> m_upRenderGraph = nullptr;


		std::vector<DrawItem> m_drawItemVec = {};


		std::shared_ptr<QuadPolygon> m_spQuadPolygon = nullptr;

		// シングルトン
	private:

		RenderContext();
		~RenderContext();

	public:

		/// <summary>
		/// シングルトン
		/// </summary>
		/// <returns>インスタンスを取得</returns>
		static RenderContext& Instance()
		{
			static RenderContext _instance;
			return _instance;
		}

	};
}