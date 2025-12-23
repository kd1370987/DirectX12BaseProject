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

//============================================================================================
//
// 初期化
//
//============================================================================================
void RenderContext::Init()
{
	// カメラ用意
	//auto _eyePos = DirectX::XMVectorSet(0.0f, 0.0f, -0.0f, 0.0f);	// 視点の位置
	auto _eyePos = DirectX::XMVectorSet(0.0f, 120.0f, 100.0f, 0.0f);	// 視点の位置
	auto _targetPos = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);	// 視点を向ける座標
	auto _upward = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);		// 上方向を表すベクトル
	auto _fov = DirectX::XMConvertToRadians(60.0f);						// 視野角

	auto _aspect = static_cast<float>(1280) / static_cast<float>(720);		// アスペクト比

	for (size_t _i = 0; _i < FRAME_BUFFER_COUNT; ++_i)
	{
		m_spCB0_Camera[_i] = std::make_shared<ConstantBuffer>(RenderingEngine::Instance().GetDevice());
		if (!m_spCB0_Camera[_i]->Create(sizeof(CBCamera)))
		{
			assert(0 && "変換行列用定数バッファの生成に失敗\n");
			return;
		}

		// 変換行列の登録
		auto* _ptr = m_spCB0_Camera[_i]->GetPtr<CBCamera>();
		DirectX::XMStoreFloat3(&_ptr->cameraPos, _eyePos);
		DirectX::XMStoreFloat4x4(&_ptr->viewMat, DirectX::XMMatrixLookAtRH(_eyePos, _targetPos, _upward));
		DirectX::XMStoreFloat4x4(&_ptr->projMat, DirectX::XMMatrixPerspectiveFovRH(_fov, _aspect, 0.3f, 1000.0f));

		//m_spCB1_Object[_i] = std::make_shared<ConstantBuffer>(sizeof(CBObject));
		m_spCB1_Object[_i] = std::make_shared<ConstantBuffer>(RenderingEngine::Instance().GetDevice());
		m_spCB1_Object[_i]->Create(sizeof(CBObject));
		auto _objPtr = m_spCB1_Object[_i]->GetPtr<CBObject>();	
		_objPtr->uvOffsetTiling = { 0.0f,0.0f,1.0f,1.0f };

		//m_spCB2_MeshTrans[_i] = std::make_shared<ConstantBuffer>(sizeof(CBMeshTrans));
		m_spCB2_MeshTrans[_i] = std::make_shared<ConstantBuffer>(RenderingEngine::Instance().GetDevice());
		m_spCB2_MeshTrans[_i]->Create(sizeof(CBMeshTrans));
		auto _meshPtr = m_spCB2_MeshTrans[_i]->GetPtr<CBMeshTrans>();
		_meshPtr->worldMat = {
			1.0f,0.0f,0.0f,0.0f,
			0.0f,1.0f,0.0f,0.0f,
			0.0f,0.0f,1.0f,0.0f,
			0.0f,0.0f,0.0f,1.0f
		};

		//m_spCB3_Material[_i] = std::make_shared<ConstantBuffer>(sizeof(CBMaterial));
		m_spCB3_Material[_i] = std::make_shared<ConstantBuffer>(RenderingEngine::Instance().GetDevice());
		m_spCB3_Material[_i]->Create(sizeof(CBMaterial));
		auto _matPtr = m_spCB3_Material[_i]->GetPtr<CBMaterial>();
		_matPtr->baseColorXYZW = { 1.0f,1.0f,1.0f,1.0f };
		_matPtr->emissiveXYZ = { 1.0f,1.0f,1.0f,0.0f };
		_matPtr->metallicRoughnessXY = { 0.0f,1.0f,0.0f,0.0f };
	}

	


	PSOManager::Instance().Init();

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

	// ディスクリプタヒープをセット
	ID3D12DescriptorHeap* _heaps[] = {
			DescriptorHeapManager::Instance().GetDescriptorCBV_SRV_UAV()->GetHeap()
	};
	_cmdList->SetDescriptorHeaps(std::size(_heaps), _heaps);

	// 定数バッファに転送
	auto* _ptr = m_spCB0_Camera[RenderingEngine::Instance().CurrentBackBufferIndex()]->GetPtr<CBCamera>();
	_ptr->cameraPos = {
		a_worldMat._41,
		a_worldMat._42,
		a_worldMat._43
	};

	// ビュー行列を計算して格納
	DirectX::XMMATRIX _wMat = DirectX::XMLoadFloat4x4(&a_worldMat);
	DirectX::XMMATRIX _vMat = DirectX::XMMatrixInverse(nullptr, _wMat);
	DirectX::XMStoreFloat4x4(&_ptr->viewMat, _vMat);

	// カメラ用定数バッファに転送
	_cmdList->SetGraphicsRootDescriptorTable(
		0,
		m_spCB0_Camera[RenderingEngine::Instance().CurrentBackBufferIndex()]->GetHandle()
	);
}

void RenderContext::SetProjectionMatrix(
	float a_fov, float a_aspect, float a_near, float a_far
)
{
	auto* _ptr = m_spCB0_Camera[RenderingEngine::Instance().CurrentBackBufferIndex()]->GetPtr<CBCamera>();
	DirectX::XMStoreFloat4x4(
		&_ptr->projMat,
		DirectX::XMMatrixPerspectiveFovRH(
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
	auto _currentIdx = RenderingEngine::Instance().CurrentBackBufferIndex();

	// ノード抽出
	auto& _dataNodes = a_modelResource->GetOriginalNodes();

	// オブジェクト単位の定数バッファをセット
	_cmdList->SetGraphicsRootDescriptorTable(
		1,
		m_spCB1_Object[_currentIdx]->GetHandle()
	);


	// 全描画用メッシュノードを描画
	for (auto& _nodeIdx : a_modelResource->GetDrawMeshNodeIndices())
	{
		DirectX::XMMATRIX _nodeTransMat = DirectX::XMLoadFloat4x4(&_dataNodes[_nodeIdx].worldTransform);
		DirectX::XMMATRIX _worldMat = a_worldMat;
		DrawMesh(
			_dataNodes[_nodeIdx].spMesh.get(),
			_nodeTransMat * _worldMat,
			a_modelResource->GetMaterials(),
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
	// コマンドリスト取得
	auto* _cmdList = RenderingEngine::Instance().GetCommandList();

	auto _currentIdx = RenderingEngine::Instance().CurrentBackBufferIndex();

	// メッシュの情報を送信
	_cmdList->IASetVertexBuffers(0,1,&a_mesh->GetVertexBuffer().View());	// 頂点バッファをセット
	_cmdList->IASetIndexBuffer(&a_mesh->GetIndexBuffer().View());			// インデックスバッファをセット
	
	// メッシュ変換行列の転送
	_cmdList->SetGraphicsRootDescriptorTable(
		2, m_spCB2_MeshTrans[_currentIdx]->GetHandle()
	);


	for (UINT _subIdx = 0; _subIdx < a_mesh->GetSubsets().size(); ++_subIdx)
	{
		// 面が一枚もない場合はスキップ
		if (a_mesh->GetSubsets()[_subIdx].faceCount == 0) continue;

		// マテリアルデータの転送
		const Material& _material = a_materials[a_mesh->GetSubsets()[_subIdx].materialNumber];
		auto _colorScale = DirectX::XMLoadFloat4(&a_colorScale);
		auto _emiScale = DirectX::XMLoadFloat3(&a_emissive);
		auto _ptr = m_spCB3_Material[_currentIdx]->GetPtr<CBMaterial>();

		// ベースカラー
		auto _baseColor = DirectX::XMLoadFloat4(&_material.baseColor);
		DirectX::XMStoreFloat4(&_ptr->baseColorXYZW, DirectX::XMVectorMultiply(_baseColor, _colorScale));

		_cmdList->SetGraphicsRootDescriptorTable(
			3, m_spCB3_Material[_currentIdx]->GetHandle()
		);

		//_cmdList->SetGraphicsRootDescriptorTable(4,_material.spBaseColorTex->GetGpuSrvHandle());
		_cmdList->SetGraphicsRootDescriptorTable(
			4,
			ResourceManager::Instance().GetTexture(_material.baseColorTexKey).lock()->GetGpuSrvHandle()
		);

		// 描画
		_cmdList->DrawIndexedInstanced(
			static_cast<UINT>(a_mesh->GetSubsets()[_subIdx].faceCount * 3),
			1,
			0,
			0,
			0
			);
	}
	
}
