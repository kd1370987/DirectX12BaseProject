#pragma once

class RootSignature;
class PipelineState;
class DescriptorHeap;

class ModelResource;
class Mesh;
struct Material;

class ConstantBuffer;

class CBAllocater;

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


	// 定数バッファ(オブジェクト単位での更新)
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
		DirectX::XMFLOAT4 baseColorXYZW			= { 1.0f,1.0f,1.0f,1.0f };
		DirectX::XMFLOAT4 emissiveXYZ			= { 0.0f,0.0f,0.0f,0.0f };
		DirectX::XMFLOAT4 metallicRoughnessXY	= { 0.0f,0.0f,0.0f,0.0f };
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
	/// モデル描画
	/// </summary>
	/// <param name="a_modelResource">モデルクラス</param>
	/// <param name="a_worldMat">モデルの行列</param>
	/// <param name="a_colorScale">色のスケール値</param>
	/// <param name="a_emissiveScale">エミッシブのスケール値</param>
	void DrawModel(
		std::shared_ptr<ModelResource> a_modelResource,
		const DirectX::XMMATRIX& a_worldMat = DirectX::XMMatrixIdentity(),
		const DirectX::XMFLOAT4& a_colorScale = { 1,1,1,1 },
		const DirectX::XMFLOAT3& a_emissiveScale = { 1,1,1 }
	);

	void DrawModel(
		std::shared_ptr<ModelResource> a_modelResource,
		const DirectX::XMFLOAT4X4& a_worldMat,
		const DirectX::XMFLOAT4& a_colorScale = { 1,1,1,1 },
		const DirectX::XMFLOAT3& a_emissiveScale = { 1,1,1 }
	);

	/// <summary>
	/// メッシュ描画
	/// </summary>
	/// <param name="a_mesh">メッシュのポインタ</param>
	/// <param name="a_worldMat">メッシュのワールド行列</param>
	/// <param name="a_colorScale">色のスケール値</param>
	/// <param name="a_emissive">エミッシブのスケール値</param>
	void DrawMesh(
		const Mesh* a_mesh,
		const DirectX::XMMATRIX& a_worldMat,
		const std::vector<Material>& a_materials,
		const DirectX::XMFLOAT4& a_colorScale = { 1,1,1,1 },
		const DirectX::XMFLOAT3& a_emissive = { 1,1,1 }
	);


private:

	// カメラ用定数バッファ
	CBCamera m_cb0_camera = {};
	
	// オブジェクト用定数バッファ
	CBObject m_cb1_object = {};
	CBMeshTrans m_cb2_MeshTrans = {};
	CBMaterial m_cb3_Material = {};

	// 定数バッファアロケーター
	std::unique_ptr<CBAllocater> m_spCBAllocater[CPU_FRAME_COUNT];

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