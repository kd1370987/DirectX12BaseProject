#include "RenderContext.h"

#include "Engine/Graphics/RenderingEngin/RenderingEngine.h"
#include "Engine/Graphics/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/Graphics/PSOManager/PSOManager.h"
#include "Engine/GPUResource/Texture/Texture.h"

#include "Engine/GPUResource/Model/Model.h"
#include "Engine/GPUResource/Model/ModelResource/Mesh/Mesh.h"
#include "Engine/GPUResource/Model/ModelResource/Node/Node.h"
#include "Engine/GPUResource/Model/ModelResource/Material/Material.h"

#include "Engine/GPUResource/RootSignature/RootSignature.h"
#include "Engine/GPUResource/PipeLineState/PipelineState.h"

#include "Engine/ResourceManager/ResourceManager.h"

#include "Engine/GPUResource/Buffer/VertexBuffer/VertexBuffer.h"
#include "Engine/GPUResource/Buffer/IndexBuffer/IndexBuffer.h"
#include "Engine/GPUResource/Buffer/ConstantBuffer/ConstantBuffer.h"


#include "Engine/CBAllocater/CBAllocater.h"
//============================================================================================
//
// 初期化
//
//============================================================================================
void RenderContext::Init()
{
	// カメラ用意
	auto _eyePos = DirectX::XMVectorSet(0.0f, 0.0f, 10.0f, 0.0f);	// 視点の位置
	auto _targetPos = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);	// 視点を向ける座標
	auto _upward = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);		// 上方向を表すベクトル
	auto _fov = DirectX::XMConvertToRadians(60.0f);						// 視野角

	auto _aspect = static_cast<float>(1280) / static_cast<float>(720);		// アスペクト比

	{
		// cb0
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
	}

	PSOManager::Instance().Init();

	for (int _i = 0; _i < CPU_FRAME_COUNT; ++_i)
	{
		
		m_spCBAllocater[_i] = std::make_unique<CBAllocater>();
		//m_spCBAllocater[_i]->Init(RenderingEngine::Instance().GetDevice());
		m_spCBAllocater[_i]->RootCBVCreate(RenderingEngine::Instance().GetDevice(), 32 * 1024 * 1024);
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
	PSOManager::Instance().SetPipelienStaet("SimplePipeline");
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
	m_spCBAllocater[_currentIdx]->ResetUse();

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
	//m_spCBAllocater[_currentIdx]->BindAndAttachData<CBCamera>(
	m_spCBAllocater[_currentIdx]->BindAndAttachDataRootCBV<CBCamera>(
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

//============================================================================================
//
// 描画
//
//============================================================================================
void RenderContext::DrawModel(
	std::shared_ptr<ModelResource> a_modelResource,
	const DirectX::XMMATRIX& a_worldMat,
	const DirectX::XMFLOAT4& a_colorScale,
	const DirectX::XMFLOAT3& a_emissiveScale
)
{
	// コマンドリスト取得
	auto* _cmdList = RenderingEngine::Instance().GetCommandList();
	auto _currentIdx = RenderingEngine::Instance().CurrentCPUFrameIndex();

	// ノード抽出
	auto& _dataNodes = a_modelResource->GetOriginalNodes();

	//m_spCBAllocater[_currentIdx]->BindAndAttachData<CBObject>(
	m_spCBAllocater[_currentIdx]->BindAndAttachDataRootCBV<CBObject>(
		_cmdList,
		1,
		m_cb1_object
	);

	// 全描画用メッシュノードを描画
	for (auto& _nodeIdx : a_modelResource->GetDrawMeshNodeIndices())
	{
		// ノードのワールド行列を計算
		DirectX::XMMATRIX _nodeTransMat = DirectX::XMLoadFloat4x4(&_dataNodes[_nodeIdx].worldTransform);
		DirectX::XMMATRIX _worldMat = _nodeTransMat * a_worldMat;
		
		// メッシュ描画
		DrawMesh(
			_dataNodes[_nodeIdx].spMesh.get(),
			_worldMat,
			a_modelResource->GetMaterials(),
			a_colorScale,
			a_emissiveScale
		);
	}
}
void RenderContext::DrawModel(std::shared_ptr<ModelResource> a_modelResource, const DirectX::XMFLOAT4X4& a_worldMat, const DirectX::XMFLOAT4& a_colorScale, const DirectX::XMFLOAT3& a_emissiveScale)
{
	DrawModel(
		a_modelResource,
		DirectX::XMLoadFloat4x4(&a_worldMat),
		a_colorScale,
		a_emissiveScale
	);
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
	//m_spCBAllocater[_currentIdx]->BindAndAttachData<CBMeshTrans>(
	m_spCBAllocater[_currentIdx]->BindAndAttachDataRootCBV<CBMeshTrans>(
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
//		m_spCBAllocater[_currentIdx]->BindAndAttachData<CBMaterial>(
		m_spCBAllocater[_currentIdx]->BindAndAttachDataRootCBV<CBMaterial>(
			_cmdList,
			3,
			m_cb3_Material
		); 

		_cmdList->SetGraphicsRootDescriptorTable(
			4,
			ResourceManager::Instance().GetTexture(_material.baseColorTexKey).lock()->GetGpuSrvHandle()
		);

		// 描画
		_cmdList->DrawIndexedInstanced(
			static_cast<UINT>(a_mesh->GetSubsets()[_subIdx].faceCount * 3),1,0,0,0
		);
	}
}

RenderContext::RenderContext()
{
}

RenderContext::~RenderContext()
{
}
