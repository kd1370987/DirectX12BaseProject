#include "RaytracingEngine.h"

#include "Engine/Raytracing/RaytracingWorld/RaytracingWorld.h"

#include "Engine/Resource/Manager/TextureManager/TextureManager.h"

#include "../../D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

#include "../../Graphics/RenderContext/RenderContext.h"
#include "../../D3D12/CBAllocater/CBAllocater.h"

#include "../RayPSO/RayPSO.h"
#include "../ShaderTable/ShaderTable.h"

void Engine::Raytracing::RayEngine::Dispatch()
{

	// UAVバリア
	auto* _pCmdList4 = D3D12Wrapper::Instance().GetCommandList4();
	auto& _tex = Engine::Resource::TextureManager::Instance().RefTexture(m_outTex);
	auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(
		_tex.GetResource()
	);

	_pCmdList4->ResourceBarrier(1, &barrier);

	// PSOとルートシグネチャセット
	_pCmdList4->SetPipelineState1(m_upPSO->Get());
	_pCmdList4->SetComputeRootSignature(m_upPSO->GetRootSig());

	// ディスクリプタヒープセット
	ID3D12DescriptorHeap* _heaps[] = {
		DescriptorHeapManager::Instance().GetCBV_SRV_UAVHeap()
	};
	_pCmdList4->SetDescriptorHeaps(1,_heaps);

	// 定数バッファをバインド
	m_camera.aspectRate = RenderContext::Instance().GetCameraAspectRate();
	m_camera.rotMat = RenderContext::Instance().GetCameraRotMat();
	m_camera.pos = RenderContext::Instance().GetCameraPOS();
	RenderContext::Instance().BindCB()->BindAndAttachDataComputeRootCBV<Camera>(
		_pCmdList4,
		0,
		m_camera
	);

	// TLASをバインド
	_pCmdList4->SetComputeRootShaderResourceView(
		1,
		m_upRayWorld->GetTLAS()
	);

	// 出力用UAVセット
	_pCmdList4->SetComputeRootDescriptorTable(
		2,
		DescriptorHeapManager::Instance().GetUAVGPUHandle(_tex.GetUAV())
	);

	// シェーダーテーブル
	const auto& _desc = m_upShaderTable->GetDispatchDesc();
	_pCmdList4->DispatchRays(
		&_desc
	);
	
	// UAVバリア
	auto _barrier = CD3DX12_RESOURCE_BARRIER::UAV(
		_tex.GetResource()
	);
	_pCmdList4->ResourceBarrier(
		1,
		&_barrier
	);
}


void Engine::Raytracing::RayEngine::RegistModel(const DirectX::XMFLOAT4X4& a_worldMat, const Engine::Resource::Handle<Resource::Model>& a_modelHandle)
{
	if(!m_upRayWorld)
	{
		m_upRayWorld = std::make_unique<RayWorld>();
	}

	// モデル登録
	m_upRayWorld->Register(a_worldMat,a_modelHandle);
}

void Engine::Raytracing::RayEngine::CommitWorld()
{
	//D3D12Wrapper::Instance().BeginFrame();

	// 出力テクスチャ作成
	m_outTex = Engine::Resource::TextureManager::Instance().CreateTexture(
		"RayOutTex",
		1280,
		720,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		Engine::Resource::TextureUsage::SRV | Engine::Resource::TextureUsage::UAV
	);

	//// UAVバリア
	//auto* _pCmdList4 = D3D12Wrapper::Instance().GetCommandList4();
	//auto& _tex = Engine::Resource::TextureManager::Instance().RefTexture(m_outTex);
	//auto _barrier = CD3DX12_RESOURCE_BARRIER::Transition(
	//	_tex.GetResource(),
	//	D3D12_RESOURCE_STATE_COMMON,
	//	D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	//);
	//_pCmdList4->ResourceBarrier(
	//	1,
	//	&_barrier
	//);

	// レイPSO作成
	m_upPSO = std::make_unique<RayPSO>();
	m_upPSO->Init();

	// レイワールドの作成
	if (!m_upRayWorld)
	{
		m_upRayWorld = std::make_unique<RayWorld>();
	}
	m_upRayWorld->Commit();
	m_upRayWorld->Create();

	// シェーダーテーブルの作成
	m_upShaderTable = std::make_unique<ShaderTable>();
	m_upShaderTable->Init(
		*m_upRayWorld.get(),
		*m_upPSO.get()
	);

	//D3D12Wrapper::Instance().EndFrame();
}

Engine::Raytracing::RayEngine::RayEngine()
{}

Engine::Raytracing::RayEngine::~RayEngine()
{}
