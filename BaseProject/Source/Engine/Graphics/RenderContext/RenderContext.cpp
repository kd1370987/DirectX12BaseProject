#include "RenderContext.h"

#include "Engine/Graphics/RenderingEngin/RenderingEngine.h"
#include "Engine/Graphics/DescriptorHeapManager/DescriptorHeapManager.h"

#include "Engine/GPUResource/Texture/Texture.h"

#include "Engine/GPUResource/Model/Model.h"
#include "Engine/GPUResource/Model/ModelResource/Mesh/Mesh.h"
#include "Engine/GPUResource/Model/ModelResource/Node/Node.h"
#include "Engine/GPUResource/Model/ModelResource/Material/Material.h"

#include "Engine/GPUResource/RootSignature/RootSignature.h"
#include "Engine/GPUResource/PipeLineState/PipelineState.h"

#include "Engine/GPUResource/Buffer/ConstantBuffer/ConstantBuffer.h"
#include "Engine/GPUResource/DescriptorHeap/DescriptorHeap.h"

#include "Engine/GPUResource/Buffer/VertexBuffer/VertexBuffer.h"
#include "Engine/GPUResource/Buffer/IndexBuffer/IndexBuffer.h"
#include "Engine/GPUResource/Buffer/ConstantBuffer/ConstantBuffer.h"

struct alignas(256) WorldView
{
	DirectX::XMMATRIX world;		// ワールド行列
	DirectX::XMMATRIX view;			// ビュー行列
	DirectX::XMMATRIX proj;			// 投影行列
};

//============================================================================================
//
// 初期化
//
//============================================================================================
void RenderContext::Init()
{
	// カメラ用意
	auto _eyePos = DirectX::XMVectorSet(0.0f, 120.0f, 75.0f, 0.0f);											// 視点の位置
	auto _targetPos = DirectX::XMVectorSet(0.0f, 120.0f, 0.0f, 0.0f);										// 視点を向ける座標
	auto _upward = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);											// 上方向を表すベクトル
	auto _fov = DirectX::XMConvertToRadians(60);															// 視野角
	auto _aspect =
		static_cast<float>(RenderingEngine::Instance().GetBackBufferWidth()) /
		static_cast<float>(RenderingEngine::Instance().GetBackBufferHeight());		// アスペクト比

	for (size_t _i = 0; _i < FRAME_BUFFER_COUNT; ++_i)
	{
		m_spCameraConstantBuffer[_i] = std::make_shared<ConstantBuffer>(sizeof(WorldView));
		if (!m_spCameraConstantBuffer[_i]->IsValid())
		{
			printf("変換行列用定数バッファの生成に失敗\n");
			return;
		}

		// 変換行列の登録
		auto _ptr = m_spCameraConstantBuffer[_i]->GetPtr<WorldView>();
		_ptr->world = DirectX::XMMatrixIdentity();
		_ptr->view = DirectX::XMMatrixLookAtRH(_eyePos, _targetPos, _upward);
		_ptr->proj = DirectX::XMMatrixPerspectiveFovRH(_fov, _aspect, 0.3f, 1000.0f);
	}

	m_spCB0_Object = std::make_shared<ConstantBuffer>(sizeof(CBObject));
	m_spCB1_MeshTrans = std::make_shared<ConstantBuffer>(sizeof(CBMeshTrans));
	m_spCB2_Material = std::make_shared<ConstantBuffer>(sizeof(CBMaterial));


	m_spRootSignature = std::make_shared<RootSignature>();

	// パイプラインステートの作成
	m_spPipelineState = std::make_shared<PipelineState>();
	const int _inputElementCount = 5;
	const D3D12_INPUT_ELEMENT_DESC _inputElements[_inputElementCount] = 
	{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // float3のPOSITION
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,			0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // float2のTEXCOORD
	{ "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,	0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // float4のCOLOR
	{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // float3のNORMAL 
	{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // float3のTANGENT
	
	};
	const D3D12_INPUT_LAYOUT_DESC _inputLayout =
	{
		_inputElements,
		_inputElementCount
	};
	m_spPipelineState->SetInputLayout(_inputLayout);
	m_spPipelineState->SetRootSignature(m_spRootSignature->Get());
#ifdef _DEBUG
	m_spPipelineState->SetVS(L"x64/Debug/SimpleVS.cso");
	m_spPipelineState->SetPS(L"x64/Debug/SimplePS.cso");
#else
	m_spPipeLineState->SetVS(L"x64/Release/SimpleVS.cso");
	m_spPipeLineState->SetPS(L"x64/Release/SimplePS.cso");
#endif
	m_spPipelineState->Create();
	if (!m_spPipelineState->IsValid())
	{
		printf("パイプラインステートの生成に失敗\n");
		return;
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
	_cmdList->SetGraphicsRootSignature(m_spRootSignature->Get());
	_cmdList->SetPipelineState(m_spPipelineState->Get());
	_cmdList->SetGraphicsRootConstantBufferView(
		0,
		m_spCameraConstantBuffer[RenderingEngine::Instance().CurrentBackBufferIndex()]->GetAddres()
	);

	
}
void RenderContext::EndSimpleRender()
{
}

//============================================================================================
//
// カメラ
//
//============================================================================================
void RenderContext::SetToShader()
{
	
}

//============================================================================================
//
// 描画
//
//============================================================================================
void RenderContext::DrawModel(
	const ModelResource& a_modelResource,
	const DirectX::XMFLOAT4X4& a_worldMat,
	const DirectX::XMFLOAT4& a_colorScale,
	const DirectX::XMFLOAT3& a_emissiveScale
)
{

	// ノード抽出
	auto& _dataNodes = a_modelResource.GetOriginalNodes();

	// 全描画用メッシュノードを描画
	for (auto& _nodeIdx : a_modelResource.GetDrawMeshNodeIndices())
	{
		DirectX::XMMATRIX _nodeTransMat = DirectX::XMLoadFloat4x4(&_dataNodes[_nodeIdx].worldTransform);
		DirectX::XMMATRIX _worldMat = DirectX::XMLoadFloat4x4(&a_worldMat);
		DrawMesh(
			_dataNodes[_nodeIdx].spMesh.get(),
			_nodeTransMat * _worldMat,
			a_modelResource.GetMaterials(),
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

	// メッシュの情報を送信
	_cmdList->IASetVertexBuffers(0,1,&a_mesh->GetVertexBuffer().View());	// 頂点バッファをセット
	_cmdList->IASetIndexBuffer(&a_mesh->GetIndexBuffer().View());			// インデックスバッファをセット
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);	// プリミティブトポロジーをセット

	// 行列情報をバッファにコピー
	auto _buffMat = m_spCB1_MeshTrans->GetPtr<CBMeshTrans>();
	DirectX::XMStoreFloat4x4(&_buffMat->worldMat, a_worldMat);
	_cmdList->SetGraphicsRootConstantBufferView(1, m_spCB1_MeshTrans->GetAddres());

	// ディスクリプタヒープ
	auto _mateHeap = DescriptorHeapManager::Instance().GetSRVHeap();
	_cmdList->SetDescriptorHeaps(1, &_mateHeap);			// ディスクリプタヒープをセット

	for (UINT _subIdx = 0; _subIdx < a_mesh->GetSubsets().size(); ++_subIdx)
	{
		// 面が一枚もない場合はスキップ
		if (a_mesh->GetSubsets()[_subIdx].faceCount == 0) continue;

		// マテリアルデータの転送
		//_cmdList->SetGraphicsRootDescriptorTable(1, );	// マテリアルテクスチャをセット	
		// 参照
		const Material& _material = a_materials[a_mesh->GetSubsets()[_subIdx].materialNumber];
		auto _colorScale = DirectX::XMLoadFloat4(&a_colorScale);
		auto _emiScale = DirectX::XMLoadFloat3(&a_emissive);
		auto _ptr = m_spCB2_Material->GetPtr<CBMaterial>();

		// ベースカラー
		auto _baseColor = DirectX::XMLoadFloat4(&_material.baseColor);
		DirectX::XMStoreFloat4(&_ptr->baseColor, DirectX::XMVectorMultiply(_baseColor, _colorScale));
		_cmdList->SetGraphicsRootDescriptorTable(1,_material.spBaseColorTex->GetGpuSrvHandle());

		// エミッシブ
		auto _emissive = DirectX::XMLoadFloat3(&_material.emissive);
		DirectX::XMStoreFloat3(&_ptr->emissive, DirectX::XMVectorMultiply(_emissive, _emiScale));
		if(_material.spEmissiveTex)
		{
			_cmdList->SetGraphicsRootDescriptorTable(1, _material.spEmissiveTex->GetGpuSrvHandle());
		}

		// メタリック
		auto _metallic = DirectX::XMLoadFloat(&_material.metallic);
		DirectX::XMStoreFloat(&_ptr->metallic, _metallic);

		// ラフネス
		auto _roughness = DirectX::XMLoadFloat(&_material.roughness);
		DirectX::XMStoreFloat(&_ptr->roughness, _roughness);
		if (_material.spMetallicRoughnessTex)
		{
			_cmdList->SetGraphicsRootDescriptorTable(1, _material.spMetallicRoughnessTex->GetGpuSrvHandle());
		}
		// ノーマル
		if (_material.spNormalTex)
		{
			_cmdList->SetGraphicsRootDescriptorTable(1, _material.spNormalTex->GetGpuSrvHandle());
			
		}

		// サブセット描画
		if (_subIdx >= (int)a_mesh->GetSubsets().size()) continue;
		if (a_mesh->GetSubsets()[_subIdx].faceCount == 0) continue;

		// 描画
		_cmdList->DrawIndexedInstanced(
			static_cast<UINT>(a_mesh->GetSubsets()[_subIdx].faceCount * 3),
			static_cast<UINT>(a_mesh->GetSubsets()[_subIdx].faceStart * 3),
			0,
			0,
			0
			);
	}
	
}
