#include "RaytracingGIPass.h"

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
	void AddRaytracingGIPass(D3D12::PipelineStateManager* a_pPSOManager, RenderGraph& a_rg, const EDrawPhase& a_phase)
	{
		struct GBufferIndex
		{
			int depth;
			int normal;
			int frameCount;
			int pad;
		};

		struct RuntimeData
		{
			Raytracing::RayPSO rayPSO;
			Raytracing::ShaderTable shaderTable;
			int frameCount = 0;
			RenderGraph* pRG;
		};
		auto _spPassData = std::make_shared<RuntimeData>();
		_spPassData->pRG = &a_rg;

		RenderPassNode _node = {};
		_node.name = "RaytracingGIPass";
		RGGlobalsPassBuilder _rpBuilder(&_node, &a_rg);

		auto* _pDevice = D3D12::D3D12Wrapper::Instance().GetDevice();

		// レイ用ルートシグネチャ
		D3D12::RootSignatureDesc _rayGlobal = {};
		_rayGlobal.isUseStaticSampler = true;
		_rayGlobal.AddRoot(D3D12::RootParameterType::RootCBV, 0);		// カメラ
		_rayGlobal.AddRoot(D3D12::RootParameterType::RootSRV, 0);		// TLAS
		_rayGlobal.AddDescriptorHeap({ {D3D12::RangeType::UAV,0} });	// 出力
		_rayGlobal.AddDescriptorHeap({ {D3D12::RangeType::SRV,1} });	// インスタンス配列
		_rayGlobal.AddDescriptorHeap({ {D3D12::RangeType::SRV,2} });	// マテリアル
		_rayGlobal.AddRoot(D3D12::RootParameterType::RootCBV, 1);		// GBufferインデックス
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
			float hitDistance;
			DXSM::Vector3 pad3_3;
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
		_psoInit.pGlobalRootSig = a_pPSOManager->Request(_rayGlobal);
		_psoInit.pHitRootSig = a_pPSOManager->Request(_hitSigInit);
		_psoInit.pRayGenRootSig = a_pPSOManager->Request(_rayGenSigInit);
		_psoInit.pMissRootSig = a_pPSOManager->Request(_missSigInit);
		_psoInit.SetPayload<Payload>();

		if (!_spPassData->rayPSO.Init(_pDevice,_psoInit))
		{
			assert(0 && "エラー！！");
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

		_rpBuilder.WriteUAV("RayGI", DXGI_FORMAT_R16G16B16A16_FLOAT, LoadOp::Clear, StoreOp::Store);

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
			_spPassData->shaderTable.CommitInstanceBindLess(_instanceVec, a_pCtx, _winOp.windowWidth, _winOp.windowHegiht);

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
			a_pCtx->BindUAVBindLess(2, _spPassData->pRG->GetUAVHandle("RayGI"));

			// GBufferIndex
			GBufferIndex _gbIdx = {};
			_gbIdx.depth = _spPassData->pRG->GetSRVHandle("Depth").GetIndex();
			_gbIdx.normal = _spPassData->pRG->GetSRVHandle("GBufferNormal").GetIndex();
			_gbIdx.frameCount = _spPassData->frameCount++;
			a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<GBufferIndex>(
				_pCmdList,
				5,
				_gbIdx
			);

			// ライト
			const AmbientData& _ambient = a_pGE->GetAmbientData();
			a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<AmbientData>(
				_pCmdList,
				6,
				_ambient
			);

			// ディスパッチ
			Raytracing::RayEngine::Instance().Dispatch(a_pCtx, _spPassData->shaderTable);
		};

		a_rg.AddPassNode(a_phase, _node);
	}
}