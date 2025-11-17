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
	/// モデル描画
	/// </summary>
	/// <param name="a_modelResource">モデルクラス</param>
	/// <param name="a_worldMat">モデルの行列</param>
	/// <param name="a_colorScale">色のスケール値</param>
	/// <param name="a_emissiveScale">エミッシブのスケール値</param>
	void DrawModel(
		const ModelResource& a_modelResource,
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

private:

	// 描画ストリーム
	std::shared_ptr<RootSignature> m_spRootSignature;		// ルートシグネチャ
	std::shared_ptr<PipelineState> m_spPipelineState;		// パイプラインステート

	// カメラ用定数バッファ
	std::shared_ptr<ConstantBuffer> m_spCameraConstantBuffer[FRAME_BUFFER_COUNT] = {nullptr};		// カメラ用定数バッファ
	std::shared_ptr<DescriptorHeap> m_spDescriptorHeap = nullptr;									// ディスクリプタヒープ

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