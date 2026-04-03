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

	// ワールドを更新
	if(!m_isCommit)
	{
		m_upRayWorld->Commit();
		m_upShaderTable->Update(*m_upRayWorld.get());

		m_isCommit = true;
	}

	// ディスクリプタヒープセット
	ID3D12DescriptorHeap* _heaps[] = {
		DescriptorHeapManager::Instance().GetCBV_SRV_UAVHeap(),
		DescriptorHeapManager::Instance().RefSamplerHeap()
	};
	_pCmdList4->SetDescriptorHeaps(ARRAYSIZE(_heaps), _heaps);


	// PSOとルートシグネチャセット
	_pCmdList4->SetPipelineState1(m_upPSO->Get());
	_pCmdList4->SetComputeRootSignature(m_upPSO->GetRootSig());


	// 定数バッファをバインド
	m_camera.aspectRate = Graphics::RenderContext::Instance().GetCameraAspectRate();
	m_camera.rotMat = Graphics::RenderContext::Instance().GetCameraRotMat();
	m_camera.pos = Graphics::RenderContext::Instance().GetCameraPOS();
	Graphics::RenderContext::Instance().BindCB()->BindAndAttachDataComputeRootCBV<Camera>(
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

	// 構造体バッファセット
	_pCmdList4->SetComputeRootDescriptorTable(
		3,
		m_upRayWorld->GetInstanceDataSRV()
	);

	// マテリアル送信
	_pCmdList4->SetComputeRootDescriptorTable(
		4,
		m_upRayWorld->GetMaterialSRV()
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

	m_isCommit = false;
}

void Engine::Raytracing::RayEngine::CommitWorld()
{
	// 出力テクスチャ作成
	m_outTex = Engine::Resource::TextureManager::Instance().CreateTexture(
		"RayOutTex",
		1280,
		720,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		Engine::Resource::TextureUsage::SRV | Engine::Resource::TextureUsage::UAV
	);

	// レイPSO作成
	m_upPSO = std::make_unique<RayPSO>();

	// グローバルルートシグネチャ構築
	RootSigInit _globalRootSigInit = {};
	_globalRootSigInit.isUseStaticSampler = true;
	_globalRootSigInit.AddRoot(RootParameterType::RootCBV, 0);		// カメラ
	_globalRootSigInit.AddRoot(RootParameterType::RootSRV, 0);		// TLAS
	_globalRootSigInit.AddDescriptorHeap({ {RangeType::UAV,0} });	// 出力
	_globalRootSigInit.AddDescriptorHeap({ {RangeType::SRV,1} });	// インスタンス配列
	_globalRootSigInit.AddDescriptorHeap({ {RangeType::SRV,2} });	// マテリアル
	_globalRootSigInit.flags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;

	// ローカルルートシグネチャ構築
	// レイジェネレーション
	RootSigInit _rayGenSigInit = {};
	_rayGenSigInit.isUseStaticSampler = false;
	_rayGenSigInit.flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
	
	// ヒットシェーダー用
	RootSigInit _hitSigInit = {};
	_hitSigInit.isUseStaticSampler = false;
	_hitSigInit.AddDescriptorHeap({ {RangeType::SRV,3},{RangeType::SRV,4},{RangeType::SRV,5},{RangeType::SRV,6} });
	_hitSigInit.AddDescriptorHeap({ {RangeType::SRV,7} });
	_hitSigInit.AddDescriptorHeap({ { RangeType::SRV,8 } });
	_hitSigInit.flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	// missシェーダー用
	RootSigInit _missSigInit = {};
	_missSigInit.isUseStaticSampler = false;
	_missSigInit.flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	// PSO初期化情報構築
	RayPSOInit _psoInit = {};
	_psoInit.shaderPass = "Asset/Shader/Ray/Raytracing.hlsl";
	_psoInit.AddShader(L"RayGen",		LocalRootSignature::RayGen,			ShaderCategory::RayGenerator);
	_psoInit.AddShader(L"Miss",			LocalRootSignature::Empty,			ShaderCategory::Miss);
	_psoInit.AddShader(L"ClosestHit",	LocalRootSignature::PBRMaterialHit, ShaderCategory::ClosestHit);
	_psoInit.AddShader(L"ShadowCHS",	LocalRootSignature::PBRMaterialHit, ShaderCategory::ClosestHit);
	_psoInit.AddShader(L"ShadowMiss",	LocalRootSignature::Empty,			ShaderCategory::Miss);
	_psoInit.AddHitGroup(L"HitGroup", L"ClosestHit");
	_psoInit.AddHitGroup(L"ShadowHitGroup", L"ShadowCHS");
	_psoInit.maxRecursionDepth = 4;
	_psoInit.opGlobalRootSigInit = _globalRootSigInit;
	_psoInit.opHitRootSigInit = _hitSigInit;
	_psoInit.opRayGenRootSigInit = _rayGenSigInit;
	_psoInit.opMissRootSigInit = _missSigInit;

	// PSO初期化
	m_upPSO->Init(_psoInit);

	// レイワールドの作成
	if (!m_upRayWorld)
	{
		m_upRayWorld = std::make_unique<RayWorld>();
	}
	m_upRayWorld->Init(_psoInit.hitGroupVec.size());

	// シェーダーテーブルの作成
	ShaderTableInit _shaderTableInit = {
		.pRayPSO = m_upPSO.get(),
		.shaderData = _psoInit.shaderDataVec,
		.hitGroup = _psoInit.hitGroupVec,
		.maxInstance = 1000,
		.maxLocalRootSize = sizeof(D3D12_GPU_DESCRIPTOR_HANDLE) * 3
	};

	m_upShaderTable = std::make_unique<ShaderTable>();
	m_upShaderTable->Init(_shaderTableInit);
}

Engine::Raytracing::RayEngine::RayEngine()
{}

Engine::Raytracing::RayEngine::~RayEngine()
{}
