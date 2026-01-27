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

class Model;

class OffScreen;

class StandardPassBase;

enum class RenderPassID
{
	Simple,
	Shadow,
	GBuffer,
	Lighting,
	PostEffect
};

// １DrawCall当たりの命令
struct RenderCommand
{
	UINT rootSigID;
	UINT psoID;

	Mesh* pMesh;				// メッシュポインタ
	UINT primitiveIndex;		// サブセット番号
	//UINT materialIndex;			// Primitiveからmaterialを引く

	DirectX::XMFLOAT4X4 worldMat;
	DirectX::XMFLOAT4	colorScale = { 1,1,1,1 };
	DirectX::XMFLOAT3	emissiveScale = { 1,1,1 };

	uint64_t sortKey;

	Resource::ID modelID;
	int nodeIndex;
};

class RenderContext
{
public:

	struct alignas(256) CBObject
	{
		// UV操作
		DirectX::XMFLOAT4 uvOffsetTiling = { 0.0f,0.0f,1.0f,1.0f };
	};

	// メッシュ座標用定数バッファ
	struct alignas(256) CBMeshTrans
	{
		DirectX::XMFLOAT4X4 worldMat;
	};

	// マテリアル単位更新用定数バッファ
	struct alignas(256) CBMaterial
	{
		DirectX::XMFLOAT4 baseColorXYZW = { 1.0f,1.0f,1.0f,1.0f };
		DirectX::XMFLOAT4 emissiveXYZ = { 0.0f,0.0f,0.0f,0.0f };
		DirectX::XMFLOAT4 metallicRoughnessXY = { 0.0f,0.0f,0.0f,0.0f };
	};


	// カメラ用定数バッファ
	struct alignas(256) CBCamera
	{
		DirectX::XMFLOAT4X4 viewMat;			// ビュー行列
		DirectX::XMFLOAT4X4 projMat;			// 射影行列
		DirectX::XMFLOAT4X4 projInvMat;			// 射影逆行列

		DirectX::XMFLOAT4 cameraPosXYZ = { 0.0f,0.0f,0.0f,0.0f };	// カメラのワールド座標
	};

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
	/// シンプルシェーダーを使った描画開始
	/// </summary>
	void BeginSimpleRender();
	/// <summary>
	/// シンプルシェーダーを使った描画終了
	/// </summary>
	void EndSimpleRender();

	/// <summary>
	/// カメラの情報を定数バッファに乗せてシェーダーに転送
	/// </summary>
	void SetToShader(
		const DirectX::XMFLOAT4X4& a_worldMat
	);

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

	void BeginOffScreen();

	void EndOffScreen();

	/// <summary>
	/// 指定した描画パスの開始
	/// </summary>
	/// <param name="a_pPass">指定パス</param>
	void BeginPass(const RenderPassID& a_pPass);

	/// <summary>
	/// BeginPassと対になっている。そのパスの終了を示す
	/// </summary>
	void EndPass();

	/// <summary>
	/// モデル描画
	/// </summary>
	/// <param name="a_modelID">モデルリソースID</param>
	/// <param name="a_worldMat">ワールド行列</param>
	/// <param name="a_colorScale">色の調整値</param>
	/// <param name="a_emissiveScale">エミッシブの調整値</param>
	void DrawModelPass(
		Resource::ID a_modelID,
		const DirectX::XMFLOAT4X4& a_worldMat,
		const DirectX::XMFLOAT4& a_colorScale = { 1,1,1,1 },
		const DirectX::XMFLOAT3& a_emissiveScale = { 1,1,1 }
	);

	/// <summary>
	/// レンダーターゲットの切り替え
	/// </summary>
	/// <param name="a_cpuHnadleVec">RTVハンドル配列</param>
	/// <param name="a_depthHandle">深度値を使うのならDSVハンドル</param>
	void ChangeRenderTarget(
		const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& a_cpuHnadleVec,
		D3D12_CPU_DESCRIPTOR_HANDLE* a_depthHandle = nullptr
	);

	void ClearRenderTarget(const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& a_cpuHnadleVec);

	void ClearDepth(const D3D12_CPU_DESCRIPTOR_HANDLE& a_depthHandle);

	//============================================================================================
	// 
	// 描画コマンド
	// 
	//============================================================================================

	void AddCommand(const RenderCommand& a_cmd);

	void ClearCommand();

	void Excute();

	void Sort();

	void DrawPrimitive(const RenderCommand& a_cmd);

	uint64_t MakeSortKey(
		uint32_t a_rootSigID,
		uint32_t a_psoID,
		uint32_t a_materialID,
		uint32_t a_meshID,
		uint32_t a_primitiveIndex
	);

private:

	// カメラ用定数バッファデータ
	CBCamera m_cb0_camera = {};

	// 1フレームで消費するリソース
	FrameResource m_frameResource[CPU_FRAME_COUNT];

	// マネージャー
	std::shared_ptr<ShaderManager>			m_spShaderManger		= nullptr;
	std::shared_ptr<RootSignatureManager>	m_spRootSigManager		= nullptr;
	std::shared_ptr<GraphicsPSOManager>		m_spGraphicsPSOManager	= nullptr;

	std::unique_ptr<OffScreen> m_upOffScreen = nullptr;

	// 描画パス
	std::unordered_map<RenderPassID, std::shared_ptr<StandardPassBase>> m_spRenderPassMap;
	StandardPassBase* m_pCurrentStandardPass = nullptr;

	// 描画コマンド
	std::vector<RenderCommand> m_commandVec;

	CBObject m_cb1_object = {};
	CBMeshTrans m_cb2_MeshTrans = {};
	CBMaterial m_cb3_Material = {};


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