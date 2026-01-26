#pragma once

#include "../StandardPassBase.h"

class SimplePass : public StandardPassBase
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

public:

	void DrawModel(
		Resource::ID a_modelID,
		const DirectX::XMFLOAT4X4& a_worldMat,
		const DirectX::XMFLOAT4& a_colorScale = { 1,1,1,1 },
		const DirectX::XMFLOAT3& a_emissiveScale = { 1,1,1 }
	) override;

	void DrawMesh(
		const Mesh* a_mesh,
		const DirectX::XMMATRIX& a_worldMat,
		const std::vector<Material>& a_materials,
		const DirectX::XMFLOAT4& a_colorScale = { 1,1,1,1 },
		const DirectX::XMFLOAT3& a_emissive = { 1,1,1 }
	) override;


private:

	void CreatePass() override;

	CBObject m_cb1_object = {};
	CBMeshTrans m_cb2_MeshTrans = {};
	CBMaterial m_cb3_Material = {};

};