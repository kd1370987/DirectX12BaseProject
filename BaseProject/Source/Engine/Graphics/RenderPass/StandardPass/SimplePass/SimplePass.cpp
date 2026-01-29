#include "SimplePass.h"

#include "Engine/D3D12/D3D12Wrapper/RenderingEngine.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/GraphicResource/GraphicResourceManager/GraphicResourceManager.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/CBAllocater/CBAllocater.h"

#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"
#include "Engine/Graphics/ShaderManager/ShaderManager.h"

void SimplePass::DrawModel(Resource::ID a_modelID, const DirectX::XMFLOAT4X4& a_worldMat, const DirectX::XMFLOAT4& a_colorScale, const DirectX::XMFLOAT3& a_emissiveScale)
{
	// レンダーコンテキスト
	auto& _ctx = RenderContext::Instance();

	// コマンドリスト取得
	auto* _cmdList = RenderingEngine::Instance().GetCommandList();

	// ノード抽出
	auto* _pModel = GraphicResourceManager::Instance().NGetModelResource(a_modelID);
	auto& _dataNodes = _pModel->originalNodes;

	_ctx.BindCB()->BindAndAttachDataRootCBV<CBObject>(
		_cmdList,
		1,
		m_cb1_object
	);

	// 全描画用メッシュノードを描画
	for (auto& _nodeIdx : _pModel->drawMeshNodeIndices)
	{
		// ノードのワールド行列を計算
		DirectX::XMMATRIX _nodeTransMat = DirectX::XMLoadFloat4x4(&_dataNodes[_nodeIdx].worldTransform);
		DirectX::XMMATRIX _wM = DirectX::XMLoadFloat4x4(&a_worldMat);
		DirectX::XMMATRIX _worldMat = _nodeTransMat * _wM;

		// メッシュ描画
		DrawMesh(
			_dataNodes[_nodeIdx].spMesh.get(),
			_worldMat,
			_pModel->materials,
			a_colorScale,
			a_emissiveScale
		);
	}
}

void SimplePass::DrawMesh(const Mesh* a_mesh, const DirectX::XMMATRIX& a_worldMat, const std::vector<Material>& a_materials, const DirectX::XMFLOAT4& a_colorScale, const DirectX::XMFLOAT3& a_emissive)
{
	if (a_mesh == nullptr)
	{
		return;
	}
	// レンダーコンテキスト
	auto& _ctx = RenderContext::Instance();

	// コマンドリスト取得
	auto* _cmdList = RenderingEngine::Instance().GetCommandList();
	auto _currentIdx = RenderingEngine::Instance().CurrentCPUFrameIndex();

	// メッシュの情報を送信
	_cmdList->IASetVertexBuffers(0, 1, &a_mesh->GetVertexBuffer().View());	// 頂点バッファをセット
	_cmdList->IASetIndexBuffer(&a_mesh->GetIndexBuffer().View());			// インデックスバッファをセット

	// メッシュ変換行列の転送
	DirectX::XMStoreFloat4x4(&m_cb2_MeshTrans.worldMat, a_worldMat);
	_ctx.BindCB()->BindAndAttachDataRootCBV<CBMeshTrans>(
		_cmdList,
		2,
		m_cb2_MeshTrans
	);

	// サブセット単位で描画
	for (UINT _subIdx = 0; _subIdx < a_mesh->GetSubsets().size(); ++_subIdx)
	{
		// 面が一枚もない場合はスキップ
		if (a_mesh->GetSubsets()[_subIdx].faceCount == 0) continue;

		// マテリアルデータの転送
		const Material& _material = a_materials[a_mesh->GetSubsets()[_subIdx].materialNumber];
		auto _colorScale = DirectX::XMLoadFloat4(&a_colorScale);
		auto _emiScale = DirectX::XMLoadFloat3(&a_emissive);

		// ベースカラー
		auto _baseColor = DirectX::XMLoadFloat4(&_material.baseColor);
		DirectX::XMStoreFloat4(&m_cb3_Material.baseColorXYZW, DirectX::XMVectorMultiply(_baseColor, _colorScale));
		m_cb3_Material.emissiveXYZ = { a_emissive.x,a_emissive.y,a_emissive.z,0 };
		m_cb3_Material.metallicRoughnessXY = { _material.metallic ,_material.roughness,0,0 };

		_ctx.BindCB()->BindAndAttachDataRootCBV<CBMaterial>(
			_cmdList,
			3,
			m_cb3_Material
		);

		// SRVの送信
		//auto _handle = _material.srvHandle.handleGPU;
		auto _handle = DescriptorHeapManager::Instance().GetSRVGPUHandle(_material.srvHandle);
		_cmdList->SetGraphicsRootDescriptorTable(
			4,
			_handle
		);

		// 描画
		UINT _faceCount = static_cast<UINT>(a_mesh->GetSubsets()[_subIdx].faceCount);
		UINT _faceStart = static_cast<UINT>(a_mesh->GetSubsets()[_subIdx].faceStart);
		_cmdList->DrawIndexedInstanced(
			_faceCount * 3, 1, _faceStart * 3, 0, 0
		);
	}
}

void SimplePass::CreatePass()
{
	// シェーダー登録
	m_shaderIDVec.clear();
	m_shaderIDVec.push_back(m_pShaderManager->Register({ "Asset/Shader/Compiled/SimpleShader/SimpleVS.cso", ShaderStage::Vertex }));
	m_shaderIDVec.push_back(m_pShaderManager->Register({ "Asset/Shader/Compiled/SimpleShader/SimplePS.cso", ShaderStage::Pixel }));

	// ルートシグネチャ登録
	m_rootSigID = m_pRootSignatureManager->Register(
		"BaseRootSig",
		{
			{ RootParameterType::RootCBV,{} },
			{ RootParameterType::RootCBV,{} },
			{ RootParameterType::RootCBV,{} },
			{ RootParameterType::RootCBV,{} },
			{ RootParameterType::DescriptorTable,
			{RangeType::SRV,RangeType::SRV,RangeType::SRV,RangeType::SRV} }
		}
	);

	// パイプラインステート登録
	D3D12_GRAPHICS_PIPELINE_STATE_DESC _psoDesc = {};
	_psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);		// ラスタライザーステート
	_psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;				// カリングなし
	_psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);				// ブレンドステートもデフォルト
	_psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);	// 深度ステンシルはデフォルトを使用
	_psoDesc.SampleMask = UINT_MAX;											// どのピクセルを描画可能にするか
	_psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;// 三角形を描画
	_psoDesc.NumRenderTargets = 1;											// 描画対象数
	_psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;				// カラーフォーマット
	_psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;								// 深度フォーマット（Zバッファの精度）
	_psoDesc.SampleDesc.Count = 1;											// サンプラーは１
	_psoDesc.SampleDesc.Quality = 0;
	_psoDesc.InputLayout = m_pShaderManager->NGet(m_shaderIDVec[0])->vsInputLayout;;
	_psoDesc.VS = m_pShaderManager->NGet(m_shaderIDVec[0])->byteCode;
	_psoDesc.PS = m_pShaderManager->NGet(m_shaderIDVec[1])->byteCode;
	_psoDesc.pRootSignature = m_pRootSignatureManager->NGet(m_rootSigID);
	m_psoID = m_pGraphicPSOManager->Register("SimplePass",_psoDesc);

	// プリミティブトポロジー設定
	m_primitive = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// cb1
	m_cb1_object.uvOffsetTiling = { 0.0f,0.0f,1.0f,1.0f };
	// cb2
	m_cb2_MeshTrans.worldMat = {
		1.0f,0.0f,0.0f,0.0f,
		0.0f,1.0f,0.0f,0.0f,
		0.0f,0.0f,1.0f,0.0f,
		0.0f,0.0f,0.0f,1.0f
	};
	/// cb3
	m_cb3_Material.baseColorXYZW = { 1.0f,1.0f,1.0f,1.0f };
	m_cb3_Material.emissiveXYZ = { 1.0f,1.0f,1.0f,0.0f };
	m_cb3_Material.metallicRoughnessXY = { 0.0f,1.0f,0.0f,0.0f };

}
