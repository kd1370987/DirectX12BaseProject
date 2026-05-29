#include "RaytracingGIPass.h"

#include "Engine/Graphics/RenderGraph/RenderGraph.h"

#include "Engine/Raytracing/RaytracingEngine/RaytracingEngine.h"
#include "../../../../D3D12/PipelineStateManager/PipelineStateManager.h"

#include "../../../../D3D12/D3DObject/CommandList/CommandList.h"
#include "../../../RenderContext/RenderContext.h"

#include "../../../../D3D12/CBAllocater/CBAllocater.h"
#include "../../../GraphicEngine.h"

namespace Engine::Graphics
{
	void Engine::Graphics::RaytracingGIPass::Excute(GraphicsEngine* a_pGE, RenderContext* a_pCtx)
	{
		auto* _pCmdList = a_pCtx->GetCurrentCmdList();

		// ---------------------------------------------------------
		// レイワールド更新・シェーダーテーブル更新
		Engine::Raytracing::RayEngine::Instance().Commit();
		const auto& _instanceVec = Raytracing::RayEngine::Instance().GetInstanceVec();
		if (_instanceVec.empty()) return;
		m_shaderTable.CommitInstanceBindLess(_instanceVec, a_pCtx);

		// ディスクリプタヒープセット
		a_pCtx->BindCopyHeapAndSumplerBindLess();

		// パイプラインとルートシグネチャセット
		_pCmdList->SetPipelineState1(m_rayPSO.Get());
		_pCmdList->SetComputeRootSignature(m_rayPSO.GetRootSig());

		// カメラバインド
		Raytracing::RayEngine::Instance().BindCamera(a_pCtx, a_pGE->GetCameraData());

		// レイワールドバインド
		Raytracing::RayEngine::Instance().BindTLAS(a_pCtx);

		// UAVをバインド
		a_pCtx->BindUAVBindLess(2, m_pRG->GetUAVHandle("RayGI"));

		// GBufferIndex
		GBufferIndex _gbIdx = {};
		_gbIdx.depth = m_pRG->GetSRVHandle("Depth").idx;
		_gbIdx.normal = m_pRG->GetSRVHandle("GBufferNormal").idx;
		_gbIdx.frameCount = m_frameCount++;
		a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<GBufferIndex>(
			_pCmdList->NGet(),
			5,
			_gbIdx
		);
		// ライト
		const AmbientData& _ambient = a_pGE->GetAmbientData();
		a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<AmbientData>(
			_pCmdList->NGet(),
			6,
			_ambient
		);

		// ディスパッチ
		Raytracing::RayEngine::Instance().Dispatch(a_pCtx, m_shaderTable);
	}
	void RaytracingGIPass::CreatePass()
	{
		// レイ用ルートシグネチャ
		D3D12::RootSignatureDesc _rayGlobal = {};
		_rayGlobal.isUseStaticSampler = true;
		_rayGlobal.AddRoot(RootParameterType::RootCBV, 0);		// カメラ
		_rayGlobal.AddRoot(RootParameterType::RootSRV, 0);		// TLAS
		_rayGlobal.AddDescriptorHeap({ {RangeType::UAV,0} });	// 出力
		_rayGlobal.AddDescriptorHeap({ {RangeType::SRV,1} });	// インスタンス配列
		_rayGlobal.AddDescriptorHeap({ {RangeType::SRV,2} });	// マテリアル
		_rayGlobal.AddRoot(RootParameterType::RootCBV, 1);		// GBufferインデックス
		_rayGlobal.AddRoot(RootParameterType::RootCBV, 10);		// ライト
		_rayGlobal.flags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;
		_rayGlobal.name = "global";
		// レイジェネレーション
		D3D12::RootSignatureDesc _rayGenSigInit = {};
		_rayGenSigInit.isUseStaticSampler = false;
		_rayGenSigInit.flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
		_rayGenSigInit.name = "gen";
		// ヒットシェーダー用
		D3D12::RootSignatureDesc _hitSigInit = {};
		_hitSigInit.isUseStaticSampler = false;
		_hitSigInit.flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
		_hitSigInit.name = "hit";
		// missシェーダー用
		D3D12::RootSignatureDesc _missSigInit = {};
		_missSigInit.isUseStaticSampler = false;
		_missSigInit.flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
		_missSigInit.name = "miss";
		// PSOの作成
		struct Payload
		{
			DXSM::Vector3 color;
			float pad;
			int hit;
			DXSM::Vector3 pad3_0;
			int depth;
			DXSM::Vector3 pad3_1;
			UINT seed;
			DXSM::Vector3 pad3_2;
		};
		Raytracing::RayPSODesc _psoInit = {};
		_psoInit.shaderPass = "Asset/Shader/Ray/GIShader/RaytracingGI.hlsl";
		_psoInit.AddShader(L"RayGen", Raytracing::LocalRootSignature::RayGen, Raytracing::ShaderCategory::RayGenerator);
		_psoInit.AddShader(L"Miss", Raytracing::LocalRootSignature::Empty, Raytracing::ShaderCategory::Miss);
		_psoInit.AddShader(L"ClosestHit", Raytracing::LocalRootSignature::PBRMaterialHit, Raytracing::ShaderCategory::ClosestHit);
		_psoInit.AddShader(L"ShadowCHS", Raytracing::LocalRootSignature::PBRMaterialHit, Raytracing::ShaderCategory::ClosestHit);
		_psoInit.AddShader(L"ShadowMiss", Raytracing::LocalRootSignature::Empty, Raytracing::ShaderCategory::Miss);
		_psoInit.AddHitGroup(L"HitGroup", L"ClosestHit");
		_psoInit.AddHitGroup(L"ShadowHitGroup", L"ShadowCHS");
		_psoInit.maxRecursionDepth = 4;
		_psoInit.pGlobalRootSig = m_pPipelineStateManager->Request(_rayGlobal);
		_psoInit.pHitRootSig = m_pPipelineStateManager->Request(_hitSigInit);
		_psoInit.pRayGenRootSig = m_pPipelineStateManager->Request(_rayGenSigInit);
		_psoInit.pMissRootSig = m_pPipelineStateManager->Request(_missSigInit);
		_psoInit.SetPayload<Payload>();

		if (!m_rayPSO.Init(_psoInit))
		{
			assert(0 && "エラー！！");
		}

		// シェーダーテーブルの作成
		Raytracing::ShaderTableInit _shaderTableInit = {
			.pRayPSO = &m_rayPSO,
			.shaderData = _psoInit.shaderDataVec,
			.hitGroup = _psoInit.hitGroupVec,
			.maxInstance = 1000,
			.maxLocalRootSize = 0
		};
		m_shaderTable.Init(_shaderTableInit);

		AddRead("GBufferNormal", AccessType::SRV, LoadOp::Load, StoreOp::Store);
		AddRead("Depth", AccessType::SRV, LoadOp::Load, StoreOp::Store);

		AddWrite("RayGI", AccessType::UAV, LoadOp::Clear, StoreOp::Store);
	}
}