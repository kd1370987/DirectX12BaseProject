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
	m_spGraphicsPSOManager->Init(50,m_spShaderManger,m_spRootSigManager);

	

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
	m_upOffScreen->SetRenderTarget(
		_cmdList,
		_currentDsvHandle
	);

	RenderingEngine::Instance().ClearRenderTargetView(m_upOffScreen->GetRTVHandle());

	// ルートシグネチャ・パイプラインステート・定数バッファをセット
	_cmdList->SetGraphicsRootSignature(m_spRootSigManager->NGet(0));
	_cmdList->SetPipelineState(m_spGraphicsPSOManager->NGet(0));
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);		// プリミティブトポロジー
	
}
void RenderContext::EndSimpleRender()
{
	auto* _cmdList = RenderingEngine::Instance().GetCommandList();
	RenderingEngine::Instance().SetBackBuffer();

	_cmdList->SetPipelineState(m_upOffScreen->m_screenPipelineDefault.Get());
	_cmdList->SetGraphicsRootSignature(m_upOffScreen->m_screenRootSignature.Get());

	_cmdList->SetDescriptorHeaps(1,m_upOffScreen->m_postProcessSRVHeap.GetAddressOf());

	auto _handle = m_upOffScreen->m_postProcessSRVHeap->GetGPUDescriptorHandleForHeapStart();
	_cmdList->SetGraphicsRootDescriptorTable(0,_handle);

	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);		// プリミティブトポロジー
	_cmdList->IASetVertexBuffers(0,1,&m_upOffScreen->m_screenVBView);
	_cmdList->DrawInstanced(4,1,0,0);
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

void RenderContext::BeginPass(const RenderPassID& a_pPass)
{
	auto _it = m_spRenderPassMap.find(a_pPass);
	if (_it == m_spRenderPassMap.end())
	{
		return;
	}
	_it->second->Begin();
	m_currentPassID = a_pPass;
	m_pCurrentStandardPass = _it->second.get();
}

void RenderContext::DrawModelPass(
	uint32_t a_modelID,
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

RenderContext::RenderContext()
{
}

RenderContext::~RenderContext()
{
}
