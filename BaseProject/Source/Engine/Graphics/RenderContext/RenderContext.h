#pragma once

class RootSignature;
class PipelineState;
class DescriptorHeap;

class Mesh;
struct Material;

class ConstantBuffer;

class CBAllocater;

class ShaderManager;
class RootSignatureManager;
class GraphicsPSOManager;

struct Model;

class OffScreen;

class RenderGraph;

enum class RenderPassID
{
	Simple,
	Shadow,
	GBuffer,
	Lighting,
	PostEffect
};

// １DrawCall当たりアイテム
struct DrawItem
{
	Material* pMaterial = nullptr;
	UINT subIdx = 0;
	Mesh* pMesh = nullptr;

	DirectX::XMFLOAT4X4* pBoneMatrices = nullptr;
	UINT boneCount = 0;

	DirectX::XMFLOAT4X4 worldMat = {};
	DirectX::XMFLOAT4	colorScale = { 1,1,1,1 };
	DirectX::XMFLOAT3	emissiveScale = { 1,1,1 };

	uint64_t sortKey = 0;
};

class RenderContext
{
public:

	// フレームで消費するリソース
	struct FrameResource
	{
		// カメラとオブジェクトの定数バッファアロケーター
		std::shared_ptr<CBAllocater> spCamAndObjectCBAllocater = nullptr;
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

	//============================================================================================
	// 
	// 描画コマンド
	// 
	//============================================================================================

	void AddItem(const RenderQueueType& a_type,const DrawItem& a_item);

	const std::vector<DrawItem>& GetItemVec(const RenderQueueType& a_type) const;

	void Excute();

	//============================================================================================
	// 
	// 描画パス構築
	// 
	//============================================================================================

	/// <summary>
	/// グラフィックスルートシグネチャをセット、前回と変更がない場合はスキップ
	/// </summary>
	/// <param name="a_rootSigID">ルートシグネチャマネージャーに登録した際のID</param>
	void SetGraphicsRootSignature(
		const Resource::ID& a_rootSigID
	);

	/// <summary>
	/// パイプラインステートをセット、前回と変更がない場合はスキップ
	/// </summary>
	/// <param name="a_rootSigID">パイプラインステートマネージャーに登録した際のID</param>
	void SetGraphicPSO(
		const Resource::ID& a_psoID
	);

	/// <summary>
	/// 1Draw当たりのオブジェクトに対する定数
	/// </summary>
	/// <param name="a_uv">UV開始位置</param>
	/// <param name="a_tile">移動値</param>
	void BindObuje(
		const DirectX::XMFLOAT2& a_uv = {0.0f,0.0f},
		const DirectX::XMFLOAT2& a_tile = {1.0f,1.0f}
	);

	/// <summary>
	/// マテリアルをSRVとして送信、その際にマテリアルの定数も送信
	/// </summary>
	/// <param name="a_pMaterial">マテリアルのポインタ</param>
	/// <param name="a_colorScale">ベース色のスケール値</param>
	/// <param name="a_emissiveScale">エミッシブのスケール値</param>
	void BindMaterial(
		Material* a_pMaterial,
		const DirectX::XMFLOAT4& a_colorScale,
		const DirectX::XMFLOAT3& a_emissiveScale
	);

	/// <summary>
	/// mesh情報を送信、前回と変更がなければスキップ
	/// </summary>
	/// <param name="a_pMesh">メッシュポインタ</param>
	/// <param name="a_worldMat">メッシュのワールド行列</param>
	void BindMesh(
		Mesh* a_pMesh,
		const DirectX::XMFLOAT4X4& a_worldMat
	);

	void BindBone(
		const DirectX::XMFLOAT4X4* a_pMatVec,
		UINT a_count
	);

	/// <summary>
	/// モデルの描画
	/// </summary>
	/// <param name="a_pMesh"></param>
	/// <param name="a_subIdx"></param>
	void Draw(
		Mesh* a_pMesh,
		UINT a_subIdx
	);

	void SetViewPort();
	void SetScissorRect();

	void Transition(
		ID3D12Resource* a_pResource,
		D3D12_RESOURCE_STATES a_before,
		D3D12_RESOURCE_STATES a_after
	);


	void ChangeBackBuffer();

	/// <summary>
	/// クワッド描画
	/// </summary>
	/// <param name="a_gpu">クワッド画面に描画するテクスチャハンドル</param>
	void DrawQuad();

	void DrawQueue(RenderQueueType a_type);

	void DrawUIQueue(RenderQueueType a_type);

private:

	// 1フレームで消費するリソース
	FrameResource m_frameResource[CPU_FRAME_COUNT] = {};

	// マネージャー
	std::shared_ptr<ShaderManager>			m_spShaderManger		= nullptr;
	std::shared_ptr<RootSignatureManager>	m_spRootSigManager		= nullptr;
	std::shared_ptr<GraphicsPSOManager>		m_spGraphicsPSOManager	= nullptr;

	std::unique_ptr<OffScreen> m_upOffScreen = nullptr;

	// 定数バッファ
	CBCamera m_cb0_camera = {};
	CBObject m_cb1_object = {};
	CBMeshTrans m_cb2_MeshTrans = {};
	CBMaterial m_cb3_Material = {};
	CBBone m_cb4_Bone = {};
	CBAmbient m_cb5_Ambient = {};
	CBUI m_cbUI = {};

	// 描画コマンド
	Resource::ID m_currentRootSigID = Resource::Limits::INVALID_ID;
	Resource::ID m_currentPSOID = Resource::Limits::INVALID_ID;
	Material* m_pCurrentMaterial = nullptr;
	Mesh* m_pCurrentMesh = nullptr;

	// ECSからの分離
	std::unordered_map<RenderQueueType, std::vector<DrawItem>> m_drawItemMap = {};

	std::unique_ptr<RenderGraph> m_upRenderGraph = nullptr;

	

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