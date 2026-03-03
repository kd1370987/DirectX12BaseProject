#include "RenderContext.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/D3D12/D3DObject/DescriptorHeap/DSVHeap/DSVHeap.h"
#include "Engine/Graphics/GraphicResource/GraphicResourceManager/GraphicResourceManager.h"

#include "Engine/D3D12//D3DObject/RootSignature/RootSignature.h"
#include "Engine/D3D12//D3DObject/PipeLineState/PipelineState.h"

#include "Engine/D3D12//D3DObject/Buffer/VertexBuffer/VertexBuffer.h"
#include "Engine/D3D12//D3DObject/Buffer/IndexBuffer/IndexBuffer.h"
#include "Engine/D3D12//D3DObject/Buffer/ConstantBuffer/ConstantBuffer.h"

#include "Engine/Graphics/ShaderManager/ShaderManager.h"
#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"

#include "Engine/D3D12/CBAllocater/CBAllocater.h"

#include "Engine/Graphics/OffScreen/OffScreen.h"

#include "../RenderGraph/RenderGraph.h"
//============================================================================================
//
// 初期化
//
//============================================================================================
void RenderContext::Init()
{
	// シェーダー用意
	m_spShaderManger = std::make_shared<ShaderManager>();
	m_spShaderManger->Init(50);
	
	// ルートシグネチャ用意
	m_spRootSigManager = std::make_shared<RootSignatureManager>();
	m_spRootSigManager->Init(10);

	m_spRootSigManager->CreateRootSig(
		"BaseRootSig",
		{
			{RootParameterType::RootCBV,{},RootSigSemantic::CameraCB,true},
			{RootParameterType::RootCBV,{},RootSigSemantic::ObjectCB,true},
			{RootParameterType::RootCBV,{},RootSigSemantic::MeshTransCB,true},
			{RootParameterType::RootCBV,{},RootSigSemantic::MaterialCB,true},
			{RootParameterType::RootCBV,{},RootSigSemantic::BoneCB,true},
			{RootParameterType::DescriptorTable,
			{RangeType::SRV,RangeType::SRV,RangeType::SRV,RangeType::SRV},
			RootSigSemantic::MaterialSRV,false}
		}
	);
	m_spRootSigManager->CreateRootSig(
		"ForwardLithingPass",
		{
			{RootParameterType::RootCBV,{},RootSigSemantic::CameraCB,true},
			{RootParameterType::RootCBV,{},RootSigSemantic::ObjectCB,true},
			{RootParameterType::RootCBV,{},RootSigSemantic::MeshTransCB,true},
			{RootParameterType::RootCBV,{},RootSigSemantic::MaterialCB,true},
			{RootParameterType::RootCBV,{},RootSigSemantic::BoneCB,true},
			{RootParameterType::RootCBV,{},RootSigSemantic::AmbientCB,true},
			{RootParameterType::DescriptorTable,
			{RangeType::SRV,RangeType::SRV,RangeType::SRV,RangeType::SRV},
			RootSigSemantic::MaterialSRV,false}
		}
	);

	m_spRootSigManager->CreateRootSig(
		"QuadRendering",
		{
			{RootParameterType::DescriptorTable,{RangeType::SRV},
			RootSigSemantic::PostScreenSRV,false}
		}
	);

	m_spRootSigManager->CreateRootSig(
		"DeferredLighting",
		{
			{RootParameterType::RootCBV,{},RootSigSemantic::CameraCB,true},
			{RootParameterType::RootCBV,{},RootSigSemantic::AmbientCB,true},
			{RootParameterType::DescriptorTable,
			{RangeType::SRV,RangeType::SRV,RangeType::SRV,RangeType::SRV,RangeType::SRV},
			RootSigSemantic::PostScreenSRV,false}
		}
	);

	m_spRootSigManager->CreateRootSig(
		"2DRootSig",
		{
			{RootParameterType::RootCBV,{},RootSigSemantic::UICB,true},
			{RootParameterType::DescriptorTable,{RangeType::SRV},RootSigSemantic::MaterialSRV,false}
		}
	);

	m_spRootSigManager->CreateRootSig(
		"DebugLine",
		{
			{RootParameterType::RootCBV,{},RootSigSemantic::CameraCB,true},
			{RootParameterType::RootCBV,{},RootSigSemantic::MeshTransCB,true},
			{RootParameterType::RootCBV,{},RootSigSemantic::BoneCB,true},
		}
	);

	// パイプラインステート用意
	m_spGraphicsPSOManager = std::make_shared<GraphicsPSOManager>();
	m_spGraphicsPSOManager->Init(50);

	

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
	
	// フレームリソース
	for (int _i = 0; _i < CPU_FRAME_COUNT; ++_i)
	{
		m_frameResource[_i].spCamAndObjectCBAllocater = std::make_shared<CBAllocater>();
		m_frameResource[_i].spCamAndObjectCBAllocater->RootCBVCreate(
			D3D12Wrapper::Instance().GetDevice(), 32 * 1024 * 1024
		);
	}

	// オフスクリーン生成
	m_upOffScreen = std::make_unique<OffScreen>();
	m_upOffScreen->CreateScreenVertex();

	m_upRenderGraph = std::make_unique<RenderGraph>();
	m_upRenderGraph->Init(
		m_spShaderManger.get(),
		m_spRootSigManager.get(),
		m_spGraphicsPSOManager.get()

	);


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

	// cb4
	for (auto& _mat : m_cb4_Bone.boneMat)
	{
		_mat = DXSM::Matrix::Identity;
	};

	m_currentPSOID = Resource::Limits::INVALID_ID;
	m_currentRootSigID = Resource::Limits::INVALID_ID;
	m_pCurrentMaterial = nullptr;
	m_pCurrentMesh = nullptr;

	// cb5
	m_cb5_Ambient.ambientLightColor = { 0.3f,0.3f,0.3f,1.0f };
	m_cb5_Ambient.directionalLightColor = { 10.0f,10.0f,10.0f,1.0f };
	m_cb5_Ambient.directionalLightDir = { -1.0f,-1.0f,-1.0f,0.0f };

	m_spQuadPolygon = std::make_shared<QuadPolygon>();
	m_spQuadPolygon->Init();
}

void RenderContext::Shutdown()
{
	for (auto& _spFR : m_frameResource)
	{
		_spFR.spCamAndObjectCBAllocater.reset();
	}

	m_upOffScreen.reset();

	m_pCurrentMaterial = nullptr;
	m_pCurrentMesh = nullptr;

	m_drawItemMap.clear();

	m_upRenderGraph->Release();
	m_upRenderGraph.reset();

	m_spShaderManger.reset();
	m_spRootSigManager.reset();
	m_spGraphicsPSOManager.reset();
}

void RenderContext::BeginFrame()
{
	
}

void RenderContext::EndFrame()
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
	auto* _cmdList = D3D12Wrapper::Instance().GetCommandList();
	UINT _currentIdx = D3D12Wrapper::Instance().CurrentCPUFrameIndex();

	// 定数バッファの初期化
	m_frameResource[_currentIdx].spCamAndObjectCBAllocater->ResetUse();

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
	DirectX::XMStoreFloat4x4(&m_cb0_camera.viewInvMat, _wMat);

	
}

void RenderContext::BindCameraCB()
{
	auto* _cmdList = D3D12Wrapper::Instance().GetCommandList();
	// レジスター番号取得
	UINT _regiIdx = 
		m_spRootSigManager->GetRegiNum(m_currentRootSigID, RootSigSemantic::CameraCB);

	// ディスクリプタヒープをセット
	ID3D12DescriptorHeap* _heaps[] = {
			DescriptorHeapManager::Instance().GetCBV_SRV_UAVHeap()
	};
	_cmdList->SetDescriptorHeaps(std::size(_heaps), _heaps);


	// ルートシグネチャにカメラCBが含まれているのなら
	if (ERR_UINT != _regiIdx)
	{
		// カメラ用定数バッファに転送
		BindCB()->BindSemanticCBV<RootSigSemantic::CameraCB>(
			_cmdList,
			_regiIdx,
			m_cb0_camera
		);
	}

	// 環境
	_regiIdx =
		m_spRootSigManager->GetRegiNum(m_currentRootSigID, RootSigSemantic::AmbientCB);
	if (ERR_UINT != _regiIdx)
	{
		BindCB()->BindSemanticCBV<RootSigSemantic::AmbientCB>(
			_cmdList,
			_regiIdx,
			m_cb5_Ambient
		);
	}
}

void RenderContext::BindCB(RootSigSemantic a_sema)
{
	// レジスター番号取得
	UINT _regiIdx =
		m_spRootSigManager->GetRegiNum(m_currentRootSigID, a_sema);
	auto* _cmdList = D3D12Wrapper::Instance().GetCommandList();

	if (ERR_UINT != _regiIdx)
	{
		switch (a_sema)
		{
		case RootSigSemantic::CameraCB:
			BindCB()->BindSemanticCBV<RootSigSemantic::CameraCB>(
				_cmdList,
				_regiIdx,
				m_cb0_camera
			);
			break;
		case RootSigSemantic::ObjectCB:
			BindCB()->BindSemanticCBV<RootSigSemantic::ObjectCB>(
				_cmdList,
				_regiIdx,
				m_cb1_object
			);
			break;
		case RootSigSemantic::MeshTransCB:
			BindCB()->BindSemanticCBV<RootSigSemantic::MeshTransCB>(
				_cmdList,
				_regiIdx,
				m_cb2_MeshTrans
			);
			break;
		case RootSigSemantic::MaterialCB:
			BindCB()->BindSemanticCBV<RootSigSemantic::MaterialCB>(
				_cmdList,
				_regiIdx,
				m_cb3_Material
			);
			break;
		case RootSigSemantic::MaterialSRV:
			break;
		case RootSigSemantic::PostScreenSRV:
			break;
		default:
			break;
		}
	}
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

CBAllocater* RenderContext::BindCB()
{
	auto _currentIdx = D3D12Wrapper::Instance().CurrentCPUFrameIndex();
	return m_frameResource[_currentIdx].spCamAndObjectCBAllocater.get();
}


void RenderContext::ChangeRenderTarget(
	const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& a_cpuHnadleVec,
	D3D12_CPU_DESCRIPTOR_HANDLE* a_depthHandle
)
{
	auto* _pCmdList = D3D12Wrapper::Instance().GetCommandList();
	
	if (a_cpuHnadleVec.empty()) return;

	_pCmdList->OMSetRenderTargets(
		static_cast<UINT>(std::size(a_cpuHnadleVec)),
		a_cpuHnadleVec.data(),
		false, 
		a_depthHandle
	);

	D3D12Wrapper::Instance().SetViewportAndRect();
}

void RenderContext::BindSRV(
	RootSigSemantic a_sema,
	const std::vector<D3D12_GPU_DESCRIPTOR_HANDLE>& a_srvHandle
)
{
	auto* _pCmdList = D3D12Wrapper::Instance().GetCommandList();

	UINT _regiIdx =
		m_spRootSigManager->GetRegiNum(m_currentRootSigID, a_sema);


	// SRVセット
	if (ERR_UINT != _regiIdx)
	{
		for(UINT _i = 0; _i < a_srvHandle.size(); ++_i)
		{
			_pCmdList->SetGraphicsRootDescriptorTable(
				_regiIdx,
				a_srvHandle[0]
			);
		}
	}
}

void RenderContext::ClearRenderTarget(const D3D12_CPU_DESCRIPTOR_HANDLE& a_cpuHandle, const DirectX::XMFLOAT4& a_colorRGBA, const UINT& a_numRects, const D3D12_RECT* a_pRects)
{
	D3D12Wrapper::Instance().ClearRenderTargetView(
		a_cpuHandle,
		a_colorRGBA,
		a_numRects,
		a_pRects
	);
}

void RenderContext::ClearDepth(const D3D12_CPU_DESCRIPTOR_HANDLE& a_depthHandle)
{
	D3D12Wrapper::Instance().ClearDepthStencilView(a_depthHandle);
}

void RenderContext::AddItem(const RenderQueueType& a_type, const DrawItem& a_item)
{
	m_drawItemMap[a_type].push_back(a_item);
}

void RenderContext::AddItem(RenderQueueType2D a_type, const DrawItem2D& a_itemVec)
{
	m_drawItem2DMap[a_type].push_back(a_itemVec);
}

const std::vector<DrawItem>& RenderContext::GetItemVec(const RenderQueueType& a_type) const
{
	auto _it = m_drawItemMap.find(a_type);
	if (_it != m_drawItemMap.end())
	{
		return _it->second;
	}
	std::vector<DrawItem> _items = {};
	return _items;
}

const std::vector<DrawItem2D>& RenderContext::GetItemVec(const RenderQueueType2D& a_type) const
{
	auto _it = m_drawItem2DMap.find(a_type);
	if (_it != m_drawItem2DMap.end())
	{
		return _it->second;
	}
	std::vector<DrawItem2D> _items = {};
	return _items;
}


void RenderContext::Excute()
{
	// バインド対象のクリア
	m_currentRootSigID = Resource::Limits::INVALID_ID;
	m_currentPSOID = Resource::Limits::INVALID_ID;
	m_pCurrentMaterial = nullptr;
	m_pCurrentMesh = nullptr;
	m_pCurrentPoly = nullptr;
	

	// レンダーパスの実行
	m_upRenderGraph->Excute(this);

	// 描画対象アイテムリストのクリア
	m_drawItemMap.clear();
	m_drawItem2DMap.clear();
}

void RenderContext::AddItem(const DrawItem& a_item)
{
	m_drawItemVec.push_back(a_item);
}

void RenderContext::SetGraphicsRootSignature(const Resource::ID& a_rootSigID)
{
	auto* _pCmdList = D3D12Wrapper::Instance().GetCommandList();

	// ルートシグネチャセット
	if (a_rootSigID != m_currentRootSigID)
	{
		_pCmdList->SetGraphicsRootSignature(m_spRootSigManager->NGet(a_rootSigID));
		m_currentRootSigID = a_rootSigID;
	}
}

void RenderContext::SetGraphicPSO(const Resource::ID& a_psoID)
{
	auto* _pCmdList = D3D12Wrapper::Instance().GetCommandList();

	// パイプラインステートセット
	if (a_psoID != m_currentPSOID)
	{
		_pCmdList->SetPipelineState(m_spGraphicsPSOManager->NGet(a_psoID));
		m_currentPSOID = a_psoID;
	}

	// プリミティブトポロジーセット
	_pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void RenderContext::BindObuje(const DirectX::XMFLOAT2& a_uv, const DirectX::XMFLOAT2& a_tile)
{
	auto* _pCmdList = D3D12Wrapper::Instance().GetCommandList();

	m_cb1_object.uvOffsetTiling.x = a_uv.x;
	m_cb1_object.uvOffsetTiling.y = a_uv.y;
	m_cb1_object.uvOffsetTiling.z = a_tile.x;
	m_cb1_object.uvOffsetTiling.w = a_tile.y;

	BindCB()->BindAndAttachDataRootCBV<CBObject>(
		_pCmdList,
		1,
		m_cb1_object
	);
}

void RenderContext::BindMaterial(
	Engine::Resource::Material* a_pMaterial,
	const DirectX::XMFLOAT4& a_colorScale, 
	const DirectX::XMFLOAT3& a_emissiveScale
)
{
	auto* _pCmdList = D3D12Wrapper::Instance().GetCommandList();

	// ベースカラー
	DXSM::Vector4 _colorScale(a_colorScale);
	DXSM::Vector4 _materialScale(a_pMaterial->baseColor);
	m_cb3_Material.baseColorXYZW = _materialScale *_colorScale;

	// エミッシブ
	DXSM::Vector3 _emissiveScale(a_emissiveScale);
	DXSM::Vector3 _materialEmissiveScale(a_pMaterial->emissive);
	DXSM::Vector3 _emiVec3 = _materialEmissiveScale * _emissiveScale;
	m_cb3_Material.emissiveXYZ = { _emiVec3.x,_emiVec3.y,_emiVec3.z,1 };

	// マテリアルラフネス
	m_cb3_Material.metallicRoughnessXY = { a_pMaterial->metallic ,a_pMaterial->roughness,0,0 };

	// マテリアルバッファバインド
	BindCB()->BindAndAttachDataRootCBV<CBMaterial>(
		_pCmdList,
		3,
		m_cb3_Material
	);

	// SRVの送信
	UINT _regiIdx =
		m_spRootSigManager->GetRegiNum(m_currentRootSigID, RootSigSemantic::MaterialSRV);
	if(a_pMaterial != m_pCurrentMaterial)
	{
		auto _handle = DescriptorHeapManager::Instance().GetSRVGPUHandle(a_pMaterial->srvHandle);
		_pCmdList->SetGraphicsRootDescriptorTable(
			_regiIdx,
			_handle
		);
		m_pCurrentMaterial = a_pMaterial;
	}
}

void RenderContext::BindMesh(Engine::Resource::Mesh* a_pMesh, const DirectX::XMFLOAT4X4& a_worldMat)
{
	auto* _pCmdList = D3D12Wrapper::Instance().GetCommandList();

	// メッシュ変換行列の転送
	m_cb2_MeshTrans.worldMat = a_worldMat;
	BindCB()->BindAndAttachDataRootCBV<CBMeshTrans>(
		_pCmdList,
		2,
		m_cb2_MeshTrans
	);

	if (a_pMesh != m_pCurrentMesh)
	{
		_pCmdList->IASetVertexBuffers(0, 1, &a_pMesh->GetVertexBuffer().View());
		_pCmdList->IASetIndexBuffer(&a_pMesh->GetIndexBuffer().View());
		m_pCurrentMesh = a_pMesh;
	}
}

void RenderContext::BindBone(const DirectX::XMFLOAT4X4* a_pMatVec,UINT a_count)
{
	// 定数バッファにコピー
	if(a_pMatVec)
	{
		std::memcpy(m_cb4_Bone.boneMat, a_pMatVec, sizeof(DirectX::XMFLOAT4X4) * a_count);
	}

	// ルートパラムインデックス確保
	UINT _regiIdx =
		m_spRootSigManager->GetRegiNum(m_currentRootSigID, RootSigSemantic::BoneCB);
	auto* _cmdList = D3D12Wrapper::Instance().GetCommandList();

	// バッファにコピー
	BindCB()->BindAndAttachDataRootCBV<CBBone>(
		_cmdList,
		_regiIdx,
		m_cb4_Bone
	);
}

void RenderContext::Draw(Engine::Resource::Mesh* a_pMesh, UINT a_subIdx)
{
	auto* _pCmdList = D3D12Wrapper::Instance().GetCommandList();

	// 描画
	UINT _faceCount = static_cast<UINT>(a_pMesh->GetSubsets()[a_subIdx].faceCount);
	UINT _faceStart = static_cast<UINT>(a_pMesh->GetSubsets()[a_subIdx].faceStart);
	_pCmdList->DrawIndexedInstanced(
		_faceCount * 3, 1, _faceStart * 3, 0, 0
	);
}

void RenderContext::SetViewPort()
{
}

void RenderContext::SetScissorRect()
{
}

void RenderContext::Transition(
	ID3D12Resource* a_pResource, 
	D3D12_RESOURCE_STATES a_before, 
	D3D12_RESOURCE_STATES a_after
)
{
	D3D12Wrapper::Instance().ResourceBarrier(
		a_pResource,
		a_before,
		a_after
	);
}

void RenderContext::ChangeBackBuffer()
{
	// レンダーターゲットをバックバッファへ切り替える
	D3D12Wrapper::Instance().SetBackBuffer();
}

void RenderContext::DrawQuad()
{
	// コマンドリストの取得
	auto* _cmdList = D3D12Wrapper::Instance().GetCommandList();

	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);		// プリミティブトポロジー
	_cmdList->IASetVertexBuffers(0, 1, &m_upOffScreen->m_screenVBView);
	_cmdList->DrawInstanced(4, 1, 0, 0);
}

void RenderContext::DrawQueue(RenderPassID a_passID, LightingType a_lightingType)
{
	for (auto& _item : m_drawItemVec)
	{
		if (_item.passID != a_passID) continue;
		if (_item.lightingType != a_lightingType) continue;

		BindObuje(
			{ 0.0f,0.0f },
			{ 1.0f,1.0f }
		);

		BindMaterial(_item.pMaterial, _item.colorScale, _item.emissiveScale);
		BindMesh(_item.pMesh, _item.worldMat);

		// ボーン行列があるのなら転送
		if (_item.pBoneMatrices)
		{
			BindBone(
				_item.pBoneMatrices,
				_item.boneCount
			);
		}

		Draw(_item.pMesh, _item.subIdx);
	}
}


void RenderContext::DrawUIQueue(RenderQueueType2D a_type)
{
	auto* _pCmdList = D3D12Wrapper::Instance().GetCommandList();

	// 描画アイテム取得
	auto& _draws = GetItemVec(a_type);
	if (_draws.size() == 0) return;
	for (auto& _item : _draws)
	{
		m_cbUI.uiMat = _item.worldMat;
		m_cbUI.color = _item.colorScale;

		BindCB()->BindAndAttachDataRootCBV<CBUI>(
			_pCmdList,
			0,
			m_cbUI
		);

		// SRVの送信
		UINT _regiIdx =
			m_spRootSigManager->GetRegiNum(m_currentRootSigID, RootSigSemantic::MaterialSRV);
		auto _handle = DescriptorHeapManager::Instance().GetSRVGPUHandle(_item.srvHandleRange);
		_pCmdList->SetGraphicsRootDescriptorTable(
			_regiIdx,
			_handle
		);

		
		// メッシュ変換行列の転送
		if(m_pCurrentPoly != m_spQuadPolygon.get())
		{
			_pCmdList->IASetVertexBuffers(0, 1, &m_spQuadPolygon->GetVBView());
			_pCmdList->IASetIndexBuffer(&m_spQuadPolygon->GetIBView());

			m_pCurrentPoly = m_spQuadPolygon.get();
		}

		// 描画
		_pCmdList->DrawIndexedInstanced(
			6, 1, 0, 0, 0
		);
	}
}

D3D12_GPU_DESCRIPTOR_HANDLE RenderContext::GetImGuiGPUHandle(const std::string& a_name)
{
	return m_upRenderGraph->GetImGuiGPUHandle(a_name);
}

std::vector<std::string> RenderContext::GetRGResourceList()
{
	return m_upRenderGraph->GetRGResourceList();
}

RenderContext::RenderContext()
{
	m_currentPSOID = Resource::Limits::INVALID_ID;
	m_currentRootSigID = Resource::Limits::INVALID_ID;
}

RenderContext::~RenderContext()
{
}
