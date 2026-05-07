#include "RaytracingEngine.h"

#include "Engine/Raytracing/RaytracingWorld/RaytracingWorld.h"

#include "Engine/Resource/Manager/TextureManager/TextureManager.h"

#include "../../D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "../../D3D12/D3DObject/CommandList/CommandList.h"

#include "../../Graphics/RenderContext/RenderContext.h"
#include "../../D3D12/CBAllocater/CBAllocater.h"

#include "../RayPSO/RayPSO.h"
#include "../ShaderTable/ShaderTable.h"

void Engine::Raytracing::RayEngine::Dispatch(Graphics::RenderContext* a_pRCT)
{
	// UAVバリア
	auto* _pCmdList4 = Engine::D3D12::D3D12Wrapper::Instance().GetCommandList4();
	auto& _tex = Engine::Resource::TextureManager::Instance().RefTexture(m_outTex);
	auto* _pCmdList = a_pRCT->GetCurrentCmdList();

	// ワールドを更新	
	m_upRayWorld->Commit();
	m_upShaderTable->CommitInstance(m_upRayWorld->GetInstnace(),a_pRCT);

	// ディスクリプタヒープセット
	ID3D12DescriptorHeap* _heaps[] = {
		a_pRCT->GetCBV_SRV_UAVHeap(),
		D3D12::DescriptorHeapManager::Instance().RefSamplerHeap()
	};
	_pCmdList->SetDescriptorHeaps(ARRAYSIZE(_heaps), _heaps);

	// PSOとルートシグネチャセット
	_pCmdList->SetPipelineState1(m_upPSO->Get());
	_pCmdList->SetComputeRootSignature(m_upPSO->GetRootSig());


	// 定数バッファをバインド
	m_camera.aspectRate = a_pRCT->GetCameraAspectRate();
	m_camera.rotMat = a_pRCT->GetCameraRotMat();
	m_camera.pos = a_pRCT->GetCameraPOS();
	a_pRCT->BindCB()->BindAndAttachDataComputeRootCBV<Camera>(
		_pCmdList->NGet(),
		0,
		m_camera
	);

	// TLASをバインド
	_pCmdList->SetComputeRootShaderResourceView(1,m_upRayWorld->GetTLAS());

	// 出力用UAVセット
	a_pRCT->BindUAV(
		2,
		D3D12::DescriptorHeapManager::Instance().GetCPU(_tex.GetUAV())
	);

	// 構造体バッファセット
	auto _gpuI = a_pRCT->GetGPUHandle(m_upRayWorld->GetInstanceDataSRVCPU());
	_pCmdList->SetComputeRootDescriptorTable(3,_gpuI);

	// マテリアル送信
	auto _gpuM = a_pRCT->GetGPUHandle(m_upRayWorld->GetMaterialSRVCPU());
	_pCmdList->SetComputeRootDescriptorTable(4, _gpuM);

	// シェーダーテーブル
	const auto& _desc = m_upShaderTable->GetDispatchDesc();
	_pCmdList->DispatchRays(&_desc);
}

void Engine::Raytracing::RayEngine::Dispatch(
	Resource::Handle<Resource::Texture> a_outHandle,
	Graphics::RenderContext* a_pRCT,
	RayPSO* a_pPSO,
	ShaderTable* a_pShaderTable
)
{
	auto& _tex = Engine::Resource::TextureManager::Instance().RefTexture(a_outHandle);
	auto* _pCmdList = a_pRCT->GetCurrentCmdList();

	// ワールドを更新	
	m_upRayWorld->Commit();
	m_upShaderTable->CommitInstance(m_upRayWorld->GetInstnace(), a_pRCT);

	// ディスクリプタヒープセット
	ID3D12DescriptorHeap* _heaps[] = {
		a_pRCT->GetCBV_SRV_UAVHeap(),
		D3D12::DescriptorHeapManager::Instance().RefSamplerHeap()
	};
	_pCmdList->SetDescriptorHeaps(ARRAYSIZE(_heaps), _heaps);

	// PSOとルートシグネチャセット
	_pCmdList->SetPipelineState1(a_pPSO->Get());
	_pCmdList->SetComputeRootSignature(a_pPSO->GetRootSig());


	// 定数バッファをバインド
	m_camera.aspectRate = a_pRCT->GetCameraAspectRate();
	m_camera.rotMat = a_pRCT->GetCameraRotMat();
	m_camera.pos = a_pRCT->GetCameraPOS();
	a_pRCT->BindCB()->BindAndAttachDataComputeRootCBV<Camera>(
		_pCmdList->NGet(),
		0,
		m_camera
	);

	// TLASをバインド
	_pCmdList->SetComputeRootShaderResourceView(1, m_upRayWorld->GetTLAS());

	// 出力用UAVセット
	a_pRCT->BindUAV(
		2,
		D3D12::DescriptorHeapManager::Instance().GetCPU(_tex.GetUAV())
	);

	// 構造体バッファセット
	//auto _gpuI = a_pRCT->GetGPUHandle(m_upRayWorld->GetInstanceDataSRVCPU());
	//_pCmdList->SetComputeRootDescriptorTable(3, _gpuI);

	//// マテリアル送信
	//auto _gpuM = a_pRCT->GetGPUHandle(m_upRayWorld->GetInstanceDataSRVCPU());
	//_pCmdList->SetComputeRootDescriptorTable(4, _gpuM);

	// シェーダーテーブル
	const auto& _desc = a_pShaderTable->GetDispatchDesc();
	_pCmdList->DispatchRays(&_desc);
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
	Resource::TextureCreateDesc _desc = {};
	_desc.name = "RayOutTex";
	_desc.width = 1280;
	_desc.height = 720;
	_desc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	_desc.usage = Engine::Resource::TextureUsage::SRV | Engine::Resource::TextureUsage::UAV;
	m_outTex = Engine::Resource::TextureManager::Instance().CreateTexture(_desc);

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

void Engine::Raytracing::RayEngine::BegineFrame()
{
}

void Engine::Raytracing::RayEngine::EndFrame()
{
	m_upRayWorld->Clear();
}

Engine::Raytracing::RayEngine::RayEngine()
{}

Engine::Raytracing::RayEngine::~RayEngine()
{}
