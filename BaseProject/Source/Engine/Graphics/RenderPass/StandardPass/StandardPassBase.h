#pragma once

class ShaderManager;
class RootSignatureManager;
class GraphicsPSOManager;

class Mesh;
struct Material;

class StandardPassBase
{
public:

	StandardPassBase() = default;
	virtual ~StandardPassBase() = default;

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="a_pShaderMana">シェーダーマネージャーポインタ</param>
	/// <param name="a_pRootSigMana">ルートシグネチャマネージャーのポインタ</param>
	/// <param name="a_pPSOMana">パイプラインステートのマネージャー</param>
	void Init(
		ShaderManager* a_pShaderMana,
		RootSignatureManager* a_pRootSigMana,
		GraphicsPSOManager* a_pPSOMana
	);

	/// <summary>
	/// 処理の開始
	/// </summary>
	virtual void Begin();

	/// <summary>
	/// モデルの描画
	/// </summary>
	virtual void DrawModel(
		Resource::ID a_modelID,
		const DirectX::XMFLOAT4X4& a_worldMat,
		const DirectX::XMFLOAT4& a_colorScale = { 1,1,1,1 },
		const DirectX::XMFLOAT3& a_emissiveScale = { 1,1,1 }
	) = 0;

	/// <summary>
	/// メッシュの描画
	/// </summary>
	virtual void DrawMesh(
		const Mesh* a_mesh,
		const DirectX::XMMATRIX& a_worldMat,
		const std::vector<Material>& a_materials,
		const DirectX::XMFLOAT4& a_colorScale = { 1,1,1,1 },
		const DirectX::XMFLOAT3& a_emissive = { 1,1,1 }
	) = 0;

	/// <summary>
	/// 処理の終了
	/// </summary>
	virtual void End();

protected:

	/// <summary>
	/// パスの作成
	/// </summary>
	virtual void CreatePass() = 0;

protected:

	ShaderManager* m_pShaderManager = nullptr;
	RootSignatureManager* m_pRootSignatureManager = nullptr;
	GraphicsPSOManager* m_pGraphicPSOManager = nullptr;


	// ルートシグネチャID
	Resource::ID m_rootSigID = Resource::Limits::MAX_STORAGE;

	// パイプラインステートID
	Resource::ID m_psoID = Resource::Limits::MAX_STORAGE;

	// シェーダーIDリスト
	std::vector<Resource::ID> m_shaderIDVec = {};

	// プリミティブトポロジー
	D3D_PRIMITIVE_TOPOLOGY m_primitive = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

};