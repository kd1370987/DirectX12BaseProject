#include "RaytracingShadowPass.h"

#include "Engine/Graphics/RenderPassRegistry/RenderPassRegistry.h"
#include "../../../../D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Raytracing/RaytracingEngine/RaytracingEngine.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/CBAllocater/CBAllocater.h"

#include "Engine/Raytracing/RayPSO/RayPSO.h"
#include "Engine/Raytracing/ShaderTable/ShaderTable.h"

#include "../../../../Option/OptionManager.h"

namespace Engine::Graphics
{
	void AddRaytracingShadowPass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
	{
		auto* _pDevice = D3D12::D3D12Wrapper::Instance().GetDevice();
		struct GBufferIndex
		{
			int depth;
			int normal;
			DirectX::XMFLOAT2 pad2;
		};

		struct RuntimeData
		{
			Raytracing::RayPSO rayPSO;
			Raytracing::ShaderTable shaderTable;
			
		};
		auto _spPassData = std::make_shared<RuntimeData>();
		

		RenderPassNode _node = {};
		_node.name = "RaytracingShadowPass";
		_node.phase = a_phase;
		RGGlobalsPassBuilder _rpBuilder(&_node);

		// レイ用ルートシグネチャ
		D3D12::RootSignatureDesc _rayGlobal = {};
		_rayGlobal.isUseStaticSampler = true;
		_rayGlobal.AddRoot(D3D12::RootParameterType::RootCBV, 0);		// カメラ
		_rayGlobal.AddRoot(D3D12::RootParameterType::RootSRV, 0);		// TLAS
		_rayGlobal.AddDescriptorHeap({ {D3D12::RangeType::UAV,0} });	// 出力
		_rayGlobal.AddRoot(D3D12::RootParameterType::RootCBV, 1);		// GBufferIndex
		_rayGlobal.AddRoot(D3D12::RootParameterType::RootCBV, 10);		// ライト
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
		Raytracing::RayPSODesc _psoInit = {};
		_psoInit.shaderPass = "Asset/Shader/Ray/RayShadowShader/RayShadowShader.hlsl";
		_psoInit.AddShader(L"RayGen", Raytracing::LocalRootSignature::RayGen, Raytracing::ShaderCategory::RayGenerator);
		_psoInit.AddShader(L"ShadowCHS", Raytracing::LocalRootSignature::PBRMaterialHit, Raytracing::ShaderCategory::ClosestHit);
		_psoInit.AddShader(L"ShadowMiss", Raytracing::LocalRootSignature::Empty, Raytracing::ShaderCategory::Miss);
		_psoInit.AddHitGroup(L"ShadowHitGroup", L"ShadowCHS");
		_psoInit.maxRecursionDepth = 1;
		_psoInit.pGlobalRootSig = a_pPSOManager->Request(_rayGlobal);
		_psoInit.pHitRootSig = a_pPSOManager->Request(_hitSigInit);
		_psoInit.pRayGenRootSig = a_pPSOManager->Request(_rayGenSigInit);
		_psoInit.pMissRootSig = a_pPSOManager->Request(_missSigInit);

		if (!_spPassData->rayPSO.Init(_pDevice,_psoInit))
		{
			ENGINE_ERRLOG(false,"PSOの初期化に失敗");
		}

		// シェーダーテーブルの作成
		Raytracing::ShaderTableInit _shaderTableInit = {
			.pRayPSO = &_spPassData->rayPSO,
			.shaderData = _psoInit.shaderDataVec,
			.hitGroup = _psoInit.hitGroupVec,
			.maxInstance = 1000,
			.maxLocalRootSize = 0
		};
		_spPassData->shaderTable.Init(_pDevice, _shaderTableInit);

		_rpBuilder.ReadSRV("GBufferNormal");
		_rpBuilder.ReadSRV("Depth");

		_rpBuilder.WriteUAV("RayShadow", DXGI_FORMAT_R8G8B8A8_UNORM, LoadOp::Clear, StoreOp::Store);

		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
		{
			auto* _pCmdList = a_pCtx->GetCurrentCmdList();

			// オプション取得
			const auto& _winOp = Engine::Option::OptionManager::GetInstance().GetWindowOption();

			// レイワールド更新・シェーダーテーブル更新
			Engine::Raytracing::RayEngine::Instance().Commit(_pCmdList);
			const auto& _instanceVec = Raytracing::RayEngine::Instance().GetInstanceVec();
			if (_instanceVec.empty()) 
			{
				return;
			}
			_spPassData->shaderTable.CommitInstanceBindLess(
				_instanceVec, 
				a_pCtx,
				_winOp.windowWidth,
				_winOp.windowHegiht
			);

			// ディスクリプタヒープセット
			a_pCtx->BindCopyHeapAndSumplerBindLess();

			// パイプラインとルートシグネチャセット
			_pCmdList->SetPipelineState1(_spPassData->rayPSO.Get());
			_pCmdList->SetComputeRootSignature(_spPassData->rayPSO.GetRootSig());

			// カメラバインド
			a_pCtx->ComputeBindRootCBV(0, a_pGE->GetCameraData());

			// レイワールドバインド
			Raytracing::RayEngine::Instance().BindTLAS(a_pCtx);

			// UAVをバインド
			a_pCtx->BindUAVBindLess(2, a_pGE->RefRenderGraph()->GetPassResource(a_passIndex, "RayShadow")->GetUAV());

			// GBufferIndex
			GBufferIndex _gbIdx = {};
			_gbIdx.depth = a_pGE->RefRenderGraph()->GetPassResource(a_passIndex, "Depth")->GetSRV().GetIndex();
			_gbIdx.normal = a_pGE->RefRenderGraph()->GetPassResource(a_passIndex, "GBufferNormal")->GetSRV().GetIndex();
			a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<GBufferIndex>(
				_pCmdList,
				3,
				_gbIdx
			);

			// ライト
			const AmbientData& _ambient = a_pGE->GetAmbientData();
			a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<AmbientData>(
				_pCmdList,
				4,
				_ambient
			);

			// ディスパッチ
			const auto& _desc = _spPassData->shaderTable.GetDispatchDesc();
			_pCmdList->DispatchRays(&_desc);
		};

		_node.phase = a_phase;
		a_pRegistry->RegisterPass(_node);
	}
}