#pragma once

class RootSignature;
class PipelineState;
class DescriptorHeap;

class ModelResource;
class Mesh;
struct Material;

class ConstantBuffer;

class RenderContext
{
public:

	// カメラ用定数バッファ
	struct cbCamera
	{
		DirectX::XMFLOAT4X4 viewMat;			// ビュー行列
		DirectX::XMFLOAT4X4 projMat;			// 射影行列
		DirectX::XMFLOAT4X4 projInvMat;			// 射影逆行列

		DirectX::XMFLOAT3 camPos;				// カメラのワールド座標
		float pad = 0;

	};


	// 定数バッファ(オブジェクト単位での更新)
	struct CBObject
	{
		// UV操作
		DirectX::XMFLOAT2 uvOffset = { 0.0f,0.0f };
		DirectX::XMFLOAT2 uvTiling = { 1.0f,1.0f };
	};

	// メッシュ座標用定数バッファ
	struct CBMeshTrans
	{
		DirectX::XMFLOAT4X4 worldMat;
	};

	// マテリアル単位更新用定数バッファ
	struct CBMaterial
	{
		DirectX::XMFLOAT4 baseColor = { 1.0f,1.0f,1.0f,1.0f };

		DirectX::XMFLOAT3 emissive = { 1.0f,1.0f,1.0f };
		float metallic = 0.0f;

		float roughness = 1.0f;
		float pad[3] = { 0.f,0.f,0.f };
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
	void SetToShader();

	/// <summary>
	/// モデル描画
	/// </summary>
	/// <param name="a_modelResource">モデルクラス</param>
	/// <param name="a_worldMat">モデルの行列</param>
	/// <param name="a_colorScale">色のスケール値</param>
	/// <param name="a_emissiveScale">エミッシブのスケール値</param>
	void DrawModel(
		const std::shared_ptr<ModelResource> a_modelResource,
		const DirectX::XMMATRIX& a_worldMat = DirectX::XMMatrixIdentity(),
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
	std::shared_ptr<ConstantBuffer> m_spCameraConstantBuffer[FRAME_BUFFER_COUNT] = {nullptr};		// カメラ用定数バッファ
	std::shared_ptr<DescriptorHeap> m_spDescriptorHeap = nullptr;									// ディスクリプタヒープ

	// オブジェクト用定数バッファ
	std::shared_ptr<ConstantBuffer> m_spCB0_Object;			// オブジェクト単位で更新
	std::shared_ptr<ConstantBuffer> m_spCB1_MeshTrans;		// メッシュ毎に更新
	std::shared_ptr<ConstantBuffer> m_spCB2_Material;		// マテリアル毎に更新


// シングルトン
private:

	RenderContext() = default;
	~RenderContext() = default;

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