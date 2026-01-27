#include "RenderContext.h"

#include "Engine/D3D12/D3D12Wrapper/RenderingEngine.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/D3D12/D3DObject/DescriptorHeap/DSVHeap/DSVHeap.h"
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
#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"

#include "Engine/D3D12/CBAllocater/CBAllocater.h"

#include "Engine/Graphics/OffScreen/OffScreen.h"

// スタンダードパス
#include "Engine/Graphics/RenderPass/StandardPass/SimplePass/SimplePass.h"
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
			RenderingEngine::Instance().GetDevice(), 32 * 1024 * 1024
		);
	}

	// オフスクリーン生成
	m_upOffScreen = std::make_unique<OffScreen>();
	m_upOffScreen->CreatePostProcessResource(*RenderingEngine::Instance().GetCurrentRenderTarget());
	m_upOffScreen->CreateScreenVertex();
	m_upOffScreen->CreateScreenPipeline();

	
	auto _spPass = std::make_shared<SimplePass>();
	m_spRenderPassMap[RenderPassID::Simple] = _spPass;
	for (auto& [_id, _spPss] : m_spRenderPassMap)
	{
		_spPss->Init(
			m_spShaderManger.get(),
			m_spRootSigManager.get(),
			m_spGraphicsPSOManager.get()
		);
	}


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

//============================================================================================
// 
// 描画準備・終了
//
//============================================================================================
void RenderContext::BeginSimpleRender()
{
	auto* _cmdList = RenderingEngine::Instance().GetCommandList();

	RenderingEngine::Instance().ResourceBarrier(
		m_upOffScreen->Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);

	auto _currentDsvHandle = DescriptorHeapManager::Instance().GetDescriptorDSV()->GetHeap()->GetCPUDescriptorHandleForHeapStart();
	auto _rtvHeapPointer = DescriptorHeapManager::Instance().GetRTVCPUHandle(m_upOffScreen->m_rtvHandle);
	ChangeRenderTarget({ _rtvHeapPointer }, &_currentDsvHandle);
	ClearRenderTarget({ _rtvHeapPointer });

	// ルートシグネチャ・パイプラインステート・定数バッファをセット
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
	m_frameResource[_currentIdx].spCamAndObjectCBAllocater->ResetUse();

	// ディスクリプタヒープをセット
	ID3D12DescriptorHeap* _heaps[] = {
			DescriptorHeapManager::Instance().GetCBV_SRV_UAVHeap()
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
	m_frameResource[_currentIdx].spCamAndObjectCBAllocater->BindAndAttachDataRootCBV<CBCamera>(
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

CBAllocater* RenderContext::BindCB()
{
	auto _currentIdx = RenderingEngine::Instance().CurrentCPUFrameIndex();
	return m_frameResource[_currentIdx].spCamAndObjectCBAllocater.get();
}

void RenderContext::BeginOffScreen()
{
	// コマンドリストの取得
	auto* _cmdList = RenderingEngine::Instance().GetCommandList();

	// オフスクリーンテクスチャをレンダーターゲットへ切り替える
	RenderingEngine::Instance().ResourceBarrier(
		m_upOffScreen->Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);

	// レンダーターゲットをセット・クリア
	auto _currentDsvHandle = DescriptorHeapManager::Instance().GetDescriptorDSV()->GetHeap()->GetCPUDescriptorHandleForHeapStart();
	auto _rtvHeapPointer = DescriptorHeapManager::Instance().GetRTVCPUHandle(m_upOffScreen->m_rtvHandle);
	ChangeRenderTarget({ _rtvHeapPointer }, &_currentDsvHandle);
	ClearRenderTarget({ _rtvHeapPointer });
}

void RenderContext::EndOffScreen()
{
	// コマンドリストの取得
	auto* _cmdList = RenderingEngine::Instance().GetCommandList();

	// レンダーターゲットをバックバッファへ切り替える
	RenderingEngine::Instance().SetBackBuffer();

	// クワッドレンダリング用のルートシグネチャとパイプラインに変更
	_cmdList->SetPipelineState(m_upOffScreen->m_screenPipelineDefault.Get());
	_cmdList->SetGraphicsRootSignature(m_upOffScreen->m_screenRootSignature.Get());

	// ディスクリプタヒープをセット
	ID3D12DescriptorHeap* _heaps[] = {
			DescriptorHeapManager::Instance().GetCBV_SRV_UAVHeap()
	};
	_cmdList->SetDescriptorHeaps(std::size(_heaps), _heaps);

	auto _handle = DescriptorHeapManager::Instance().GetSRVGPUHandle(m_upOffScreen->m_srvRange);
	_cmdList->SetGraphicsRootDescriptorTable(0, _handle);

	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);		// プリミティブトポロジー
	_cmdList->IASetVertexBuffers(0, 1, &m_upOffScreen->m_screenVBView);
	_cmdList->DrawInstanced(4, 1, 0, 0);
}

void RenderContext::BeginPass(const RenderPassID& a_pPass)
{
	auto _it = m_spRenderPassMap.find(a_pPass);
	if (_it == m_spRenderPassMap.end())
	{
		return;
	}
	_it->second->Begin();
	m_pCurrentStandardPass = _it->second.get();
}

void RenderContext::DrawModelPass(
	Resource::ID a_modelID,
	const DirectX::XMFLOAT4X4& a_worldMat,
	const DirectX::XMFLOAT4& a_colorScale, 
	const DirectX::XMFLOAT3& a_emissiveScale
)
{
	if (!m_pCurrentStandardPass) return;

	m_pCurrentStandardPass->DrawModel(
		a_modelID,
		a_worldMat,
		a_colorScale,
		a_emissiveScale
	);
}

void RenderContext::EndPass()
{
	if (!m_pCurrentStandardPass) return;

	m_pCurrentStandardPass->End();
	m_pCurrentStandardPass = nullptr;
}

void RenderContext::ChangeRenderTarget(
	const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& a_cpuHnadleVec,
	D3D12_CPU_DESCRIPTOR_HANDLE* a_depthHandle
)
{
	auto* _pCmdList = RenderingEngine::Instance().GetCommandList();
	
	_pCmdList->OMSetRenderTargets(
		std::size(a_cpuHnadleVec),
		a_cpuHnadleVec.data(),
		false, 
		a_depthHandle
	);
}

void RenderContext::ClearRenderTarget(const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& a_cpuHnadleVec)
{
	for (auto& _handle : a_cpuHnadleVec)
	{
		RenderingEngine::Instance().ClearRenderTargetView(_handle);
	}
}

void RenderContext::ClearDepth(const D3D12_CPU_DESCRIPTOR_HANDLE& a_depthHandle)
{
	RenderingEngine::Instance().ClearDepthStencilView(a_depthHandle);
}

void RenderContext::AddCommand(const RenderCommand& a_cmd)
{
	m_commandVec.push_back(a_cmd);
}

void RenderContext::ClearCommand()
{
	m_commandVec.clear();
	m_commandVec.reserve(100);
}

void RenderContext::Excute()
{
	auto* _cmdList = RenderingEngine::Instance().GetCommandList();

	Sort();

	UINT _currentRoot = UINT_MAX;
	UINT _currentPSO = UINT_MAX;
	Mesh* _pMesh = nullptr;

	for (auto& _cmd : m_commandVec)
	{
		// ルートシグネチャセット
		if (_cmd.rootSigID != _currentRoot)
		{
			_cmdList->SetGraphicsRootSignature(m_spRootSigManager->NGet(_cmd.rootSigID));
			_currentRoot = _cmd.rootSigID;
		}

		// パイプラインステートセット
		if (_cmd.psoID != _currentPSO)
		{
			_cmdList->SetPipelineState(m_spGraphicsPSOManager->NGet(_cmd.psoID));
			_currentPSO = _cmd.psoID;
		}

		// 描画
		if (_cmd.pMesh != _pMesh)
		{
			_cmdList->IASetVertexBuffers(0,1,&_cmd.pMesh->GetVertexBuffer().View());
			_cmdList->IASetIndexBuffer(&_cmd.pMesh->GetIndexBuffer().View());
			_pMesh = _cmd.pMesh;
		}
		auto _model = GraphicResourceManager::Instance().NGetModelResource(_cmd.modelID);

		BindCB()->BindAndAttachDataRootCBV<CBObject>(
			_cmdList,
			1,
			m_cb1_object
		);

		// ノードのワールド行列を計算
		auto& _dataNodes = _model->originalNodes;
		DirectX::XMMATRIX _nodeTransMat = DirectX::XMLoadFloat4x4(&_dataNodes[_cmd.nodeIndex].worldTransform);
		DirectX::XMMATRIX _wM = DirectX::XMLoadFloat4x4(&_cmd.worldMat);
		DirectX::XMMATRIX _worldMat = _nodeTransMat * _wM;

		// メッシュ変換行列の転送
		DirectX::XMStoreFloat4x4(&m_cb2_MeshTrans.worldMat, _worldMat);
		BindCB()->BindAndAttachDataRootCBV<CBMeshTrans>(
			_cmdList,
			2,
			m_cb2_MeshTrans
		);

		DrawPrimitive(_cmd);
	}
}

void RenderContext::Sort()
{
	auto& _src = m_commandVec;

	for (auto& _cmd : _src)
	{
		_cmd.sortKey = MakeSortKey(
			_cmd.rootSigID,
			_cmd.psoID,
			0,
			0,
			0
		);
	}

	std::sort(
		m_commandVec.begin(),
		m_commandVec.end(),
		[](const RenderCommand& a_a, const RenderCommand& a_b)
		{
			return a_a.sortKey < a_b.sortKey;
		}
	);
}

void RenderContext::DrawPrimitive(const RenderCommand& a_cmd)
{
	auto* _cmdList = RenderingEngine::Instance().GetCommandList();
	auto _model = GraphicResourceManager::Instance().NGetModelResource(a_cmd.modelID);

	auto* _pMesh = a_cmd.pMesh;
	auto _subIdx = a_cmd.primitiveIndex;
	auto& _materials = _model->materials;
	auto& _colorScale4 = a_cmd.colorScale;
	auto& _emissiveScale = a_cmd.emissiveScale;

	// 面が一枚もない場合はスキップ
	if (_pMesh->GetSubsets()[_subIdx].faceCount == 0) return;

	// マテリアルデータの転送
	const Material& _material = _materials[_pMesh->GetSubsets()[_subIdx].materialNumber];
	auto _colorScale = DirectX::XMLoadFloat4(&_colorScale4);
	auto _emiScale = DirectX::XMLoadFloat3(&_emissiveScale);

	// ベースカラー
	auto _baseColor = DirectX::XMLoadFloat4(&_material.baseColor);
	DirectX::XMStoreFloat4(&m_cb3_Material.baseColorXYZW, DirectX::XMVectorMultiply(_baseColor, _colorScale));
	m_cb3_Material.emissiveXYZ = { _emissiveScale.x,_emissiveScale.y,_emissiveScale.z,0 };
	m_cb3_Material.metallicRoughnessXY = { _material.metallic ,_material.roughness,0,0 };

	BindCB()->BindAndAttachDataRootCBV<CBMaterial>(
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
	UINT _faceCount = static_cast<UINT>(_pMesh->GetSubsets()[_subIdx].faceCount);
	UINT _faceStart = static_cast<UINT>(_pMesh->GetSubsets()[_subIdx].faceStart);
	_cmdList->DrawIndexedInstanced(
		_faceCount * 3, 1, _faceStart * 3, 0, 0
	);
}

uint64_t RenderContext::MakeSortKey(
	uint32_t a_rootSigID,
	uint32_t a_psoID,
	uint32_t a_materialID, 
	uint32_t a_meshID,
	uint32_t a_primitiveIndex
)
{

	// 重要度の高い順に上位ビットへ配置
	uint64_t _key = 0;
	_key |= uint64_t(a_rootSigID & 0xFF)		<< 56;	// 8bit
	_key |= uint64_t(a_psoID & 0xFFF)			<< 44;	// 12bit
	_key |= uint64_t(a_materialID & 0xFFFF)		<< 28;	// 16bit
	_key |= uint64_t(a_meshID & 0xFFFFF)		<< 8;	// 20bit
	_key |= uint64_t(a_primitiveIndex & 0xFF);			// 8bit

	return _key;
}

RenderContext::RenderContext()
{
}

RenderContext::~RenderContext()
{
}
