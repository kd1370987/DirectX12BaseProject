#include "RenderContext.h"

#include "Engine/Graphics/RenderingEngin/RenderingEngine.h"
#include "Engine/Graphics/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/GraphicResource/GraphicResourceManager/GraphicResourceManager.h"
#include "Engine/GraphicResource/Resource/Texture/Texture.h"

#include "Engine/GraphicResource/Resource/Model/Model.h"
#include "Engine/GraphicResource/Resource/Model/ModelResource/Mesh/Mesh.h"
#include "Engine/GraphicResource/Resource/Model/ModelResource/Node/Node.h"
#include "Engine/GraphicResource/Resource/Model/ModelResource/Material/Material.h"

#include "Engine/D3D12//D3DObject/RootSignature/RootSignature.h"
#include "Engine/D3D12//D3DObject/PipeLineState/PipelineState.h"

#include "Engine/D3D12//D3DObject/Buffer/VertexBuffer/VertexBuffer.h"
#include "Engine/D3D12//D3DObject/Buffer/IndexBuffer/IndexBuffer.h"
#include "Engine/D3D12//D3DObject/Buffer/ConstantBuffer/ConstantBuffer.h"

#include "Engine/Graphics/ShaderManager/ShaderManager.h"
#include "Engine/Graphics/RootSignatureManager/RootSignatureManager.h"
#include "Engine/Graphics/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"

#include "Engine/CBAllocater/CBAllocater.h"
//============================================================================================
//
// 初期化
//
//============================================================================================
void RenderContext::Init()
{
	// シェーダー用意
	m_spShaderManger = std::make_shared<ShaderManager>();
	ShaderID _vsID = m_spShaderManger->Register("x64/Debug/SimpleVS.cso", ShaderStage::Vertex);
	ShaderID _psID = m_spShaderManger->Register("x64/Debug/SimplePS.cso", ShaderStage::Pixel);

	// ルートシグネチャ用意
	m_spRootSigManager = std::make_shared<RootSignatureManager>();
	m_spRootSigManager->Init();
	

	// パイプラインステート用意
	m_spGraphicsPSOManager = std::make_shared<GraphicsPSOManager>();
	m_spGraphicsPSOManager->Init(m_spShaderManger,m_spRootSigManager);

	PSOSetting _psoSetting = {};
	_psoSetting.rootsignatureID = 0;
	_psoSetting.vsStage = _vsID;
	_psoSetting.psStage = _psID;

	m_spGraphicsPSOManager->Register(_psoSetting);

	// cb0
	auto _eyePos = DirectX::XMVectorSet(0.0f, 0.0f, 10.0f, 0.0f);	// 視点の位置
	auto _targetPos = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);	// 視点を向ける座標
	auto _upward = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);		// 上方向を表すベクトル
	constexpr float _fovF = 60.0f;
	constexpr auto _fov = DirectX::XMConvertToRadians(_fovF);						// 視野角

	auto _aspect = static_cast<float>(1280) / static_cast<float>(720);		// アスペクト比
	DirectX::XMStoreFloat4(&m_cb0_camera.cameraPosXYZ, _eyePos);
	DirectX::XMStoreFloat4x4(&m_cb0_camera.viewMat, DirectX::XMMatrixLookAtLH(_eyePos, _targetPos, _upward));
	DirectX::XMStoreFloat4x4(&m_cb0_camera.projMat, DirectX::XMMatrixPerspectiveFovLH(_fov, _aspect, 0.3f, 1000.0f));
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

	// フレームリソース
	for (int _i = 0; _i < CPU_FRAME_COUNT; ++_i)
	{
		m_frameResource[_i].upCamAndObjectCBAllocater = std::make_unique<CBAllocater>();
		m_frameResource[_i].upCamAndObjectCBAllocater->RootCBVCreate(
			RenderingEngine::Instance().GetDevice(), 32 * 1024 * 1024
		);
	}
}

//============================================================================================
// 
// 描画準備・終了
//
//============================================================================================
void RenderContext::BeginSimpleRender()
{
	// ルートシグネチャ・パイプラインステート・定数バッファをセット
	auto* _cmdList = RenderingEngine::Instance().GetCommandList();
	_cmdList->SetGraphicsRootSignature(m_spRootSigManager->NGet(0));
	_cmdList->SetPipelineState(m_spGraphicsPSOManager->NGet(0));
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);		// プリミティブトポロジー
	
}
void RenderContext::EndSimpleRender()
{
}

//============================================================================================
//
// カメラ
//
//============================================================================================
void RenderContext::SetToShader(
	const DirectX::XMFLOAT4X4& a_worldMat
)
{
	// コマンドリスト取得
	auto* _cmdList = RenderingEngine::Instance().GetCommandList();
	UINT _currentIdx = RenderingEngine::Instance().CurrentCPUFrameIndex();

	// 定数バッファの初期化
	m_frameResource[_currentIdx].upCamAndObjectCBAllocater->ResetUse();

	// ディスクリプタヒープをセット
	ID3D12DescriptorHeap* _heaps[] = {
			DescriptorHeapManager::Instance().GetDescriptorCBV_SRV_UAV()->GetHeap()
	};
	_cmdList->SetDescriptorHeaps(std::size(_heaps), _heaps);

	// カメラの位置を更新
	m_cb0_camera.cameraPosXYZ = {
		a_worldMat._41,
		a_worldMat._42,
		a_worldMat._43,
		0.0f
	};

	// ビュー行列を計算して格納
	DirectX::XMMATRIX _wMat = DirectX::XMLoadFloat4x4(&a_worldMat);
	DirectX::XMMATRIX _vMat = DirectX::XMMatrixInverse(nullptr, _wMat);
	DirectX::XMStoreFloat4x4(&m_cb0_camera.viewMat, _vMat);

	// カメラ用定数バッファに転送
	m_frameResource[_currentIdx].upCamAndObjectCBAllocater->BindAndAttachDataRootCBV<CBCamera>(
		_cmdList,
		0,
		m_cb0_camera
	);
}

void RenderContext::SetProjectionMatrix(
	float a_fov, float a_aspect, float a_near, float a_far
)
{	
	DirectX::XMStoreFloat4x4(
		&m_cb0_camera.projMat,
		DirectX::XMMatrixPerspectiveFovLH(
			a_fov,
			a_aspect,
			a_near,
			a_far
		)
	);
}

void RenderContext::SetProjectionMatrix(DirectX::XMFLOAT4X4 a_projMat)
{
	m_cb0_camera.projMat = a_projMat;
}

//============================================================================================
//
// 描画
//
//============================================================================================
void RenderContext::DrawModel(uint32_t a_modelID, const DirectX::XMFLOAT4X4& a_worldMat, const DirectX::XMFLOAT4& a_colorScale, const DirectX::XMFLOAT3& a_emissiveScale)
{
	const Model* _pModel = GraphicResourceManager::Instance().NGetModelResource(a_modelID);
	DrawModel(
		_pModel,
		DirectX::XMLoadFloat4x4(&a_worldMat),
		a_colorScale,
		a_emissiveScale
	);
}
void RenderContext::DrawModel(
	const Model* a_pModel, 
	const DirectX::XMMATRIX& a_worldMat , 
	const DirectX::XMFLOAT4& a_colorScale, const DirectX::XMFLOAT3& a_emissiveScale)
{
	// コマンドリスト取得
	auto* _cmdList = RenderingEngine::Instance().GetCommandList();
	auto _currentIdx = RenderingEngine::Instance().CurrentCPUFrameIndex();

	// ノード抽出
	auto& _dataNodes = a_pModel->originalNodes;

	m_frameResource[_currentIdx].upCamAndObjectCBAllocater->BindAndAttachDataRootCBV<CBObject>(
		_cmdList,
		1,
		m_cb1_object
	);

	// 全描画用メッシュノードを描画
	for (auto& _nodeIdx : a_pModel->drawMeshNodeIndices)
	{
		// ノードのワールド行列を計算
		DirectX::XMMATRIX _nodeTransMat = DirectX::XMLoadFloat4x4(&_dataNodes[_nodeIdx].worldTransform);
		DirectX::XMMATRIX _worldMat = _nodeTransMat * a_worldMat;

		// メッシュ描画
		DrawMesh(
			_dataNodes[_nodeIdx].spMesh.get(),
			_worldMat,
			a_pModel->materials,
			a_colorScale,
			a_emissiveScale
		);
	}
}
void RenderContext::DrawMesh(
	const Mesh* a_mesh,
	const DirectX::XMMATRIX& a_worldMat,
	const std::vector<Material>& a_materials,
	const DirectX::XMFLOAT4& a_colorScale,
	const DirectX::XMFLOAT3& a_emissive
)
{
	if (a_mesh == nullptr)
	{
		return;
	}

	// コマンドリスト取得
	auto* _cmdList = RenderingEngine::Instance().GetCommandList();
	auto _currentIdx = RenderingEngine::Instance().CurrentCPUFrameIndex();

	// メッシュの情報を送信
	_cmdList->IASetVertexBuffers(0,1,&a_mesh->GetVertexBuffer().View());	// 頂点バッファをセット
	_cmdList->IASetIndexBuffer(&a_mesh->GetIndexBuffer().View());			// インデックスバッファをセット
	
	// メッシュ変換行列の転送
	DirectX::XMStoreFloat4x4(&m_cb2_MeshTrans.worldMat, a_worldMat);
	m_frameResource[_currentIdx].upCamAndObjectCBAllocater->BindAndAttachDataRootCBV<CBMeshTrans>(
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

		m_frameResource[_currentIdx].upCamAndObjectCBAllocater->BindAndAttachDataRootCBV<CBMaterial>(
			_cmdList,
			3,
			m_cb3_Material
		); 

		// SRVの送信
		auto _handle = _material.srvHandle.handleGPU;
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

RenderContext::RenderContext()
{
}

RenderContext::~RenderContext()
{
}
