#pragma once

class ModelResource;
class Mesh;

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
		const DirectX::XMFLOAT4X4& a_worldMat,
		const DirectX::XMFLOAT4& a_colorScale = { 1,1,1,1 },
		const DirectX::XMFLOAT3& a_emissive = { 1,1,1 }
	);

private:

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