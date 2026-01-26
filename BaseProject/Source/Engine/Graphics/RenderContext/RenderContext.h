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

class RenderContext
{
public:

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

	void BeginPass(const RenderPassID& a_pPass);
	void DrawModelPass(
		uint32_t a_modelID,
		const DirectX::XMFLOAT4X4& a_worldMat,
		const DirectX::XMFLOAT4& a_colorScale = { 1,1,1,1 },
		const DirectX::XMFLOAT3& a_emissiveScale = { 1,1,1 }
	);
	void EndPass();



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
	RenderPassID m_currentPassID;

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