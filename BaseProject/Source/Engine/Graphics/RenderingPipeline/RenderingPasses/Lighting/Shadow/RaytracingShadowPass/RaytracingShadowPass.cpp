#include "RaytracingShadowPass.h"

#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/Graphics/RenderingPipeline/RenderGraph/RenderGraph.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"
#include "Engine/Raytracing/RaytracingEngine/RaytracingEngine.h"

namespace Engine::Graphics::Pipeline
{
	void RaytracingShadowPass::SetupSlots()
	{
		// スロットは依存関係とバリアのために宣言する。
		// ルートパラメータ番号は付けない(バインドは自前でバインドレスに行うため)
		DeclareInput("Normal", EAccessType::SRV);
		DeclareInput("Depth", EAccessType::SRV);

		Slot& _out = DeclareOutput("Shadow", "RayShadow", DXGI_FORMAT_R8G8B8A8_UNORM,
			EAccessType::UAV);
		_out.loadOp = ELoadOp::Clear;
	}

	void RaytracingShadowPass::Compile(const PassContext& a_context)
	{
		m_isReady = false;
		if (!a_context.pGraphicsEngine) return;

		auto* _pPSOManager = a_context.pGraphicsEngine->RefPipelineStateManager();
		if (!_pPSOManager) return;

		auto* _pDevice = D3D12::D3D12Wrapper::Instance().GetDevice();
		if (!_pDevice) return;

		// ---- レイ用ルートシグネチャ ----
		D3D12::RootSignatureDesc _rayGlobal = {};
		_rayGlobal.isUseStaticSampler = true;
		_rayGlobal.AddRoot(D3D12::RootParameterType::RootCBV, 0);		// カメラ
		_rayGlobal.AddRoot(D3D12::RootParameterType::RootSRV, 0);		// TLAS
		_rayGlobal.AddDescriptorHeap({ {D3D12::RangeType::UAV,0} });	// 出力
		_rayGlobal.AddRoot(D3D12::RootParameterType::RootCBV, 1);		// GBufferIndex
		_rayGlobal.AddRoot(D3D12::RootParameterType::RootCBV, 10);		// 主光源
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

		// ---- PSOの作成 ----
		Raytracing::RayPSODesc _psoInit = {};
		_psoInit.shaderPass = "Asset/Shader/Source/Raytracing/Shadow/RayShadowShader.hlsl";
		_psoInit.AddShader(L"RayGen", Raytracing::LocalRootSignature::RayGen, Raytracing::ShaderCategory::RayGenerator);
		_psoInit.AddShader(L"ShadowCHS", Raytracing::LocalRootSignature::PBRMaterialHit, Raytracing::ShaderCategory::ClosestHit);
		_psoInit.AddShader(L"ShadowMiss", Raytracing::LocalRootSignature::Empty, Raytracing::ShaderCategory::Miss);
		_psoInit.AddHitGroup(L"ShadowHitGroup", L"ShadowCHS");
		_psoInit.maxRecursionDepth = 1;

		// ルートシグネチャはハンドルのまま渡す。実体を引くのは RayPSO::Init の中
		_psoInit.globalRootSig = _pPSOManager->Request(_rayGlobal);
		_psoInit.hitRootSig    = _pPSOManager->Request(_hitSigInit);
		_psoInit.rayGenRootSig = _pPSOManager->Request(_rayGenSigInit);
		_psoInit.missRootSig   = _pPSOManager->Request(_missSigInit);

		if (!m_rayPSO.Init(_pDevice, _pPSOManager, _psoInit))
		{
			ENGINE_WARNING("[RaytracingShadowPass] レイトレPSOの初期化に失敗しました");
			return;
		}

		// ---- シェーダーテーブルの作成 ----
		Raytracing::ShaderTableInit _shaderTableInit = {
			.pRayPSO = &m_rayPSO,
			.shaderData = _psoInit.shaderDataVec,
			.hitGroup = _psoInit.hitGroupVec,
			.maxInstance = 1000,
			.maxLocalRootSize = 0
		};
		m_shaderTable.Init(_pDevice, _shaderTableInit);

		m_isReady = true;
	}

	void RaytracingShadowPass::Update(const PassContext& a_context)
	{
		if (!m_isReady) return;

		RenderContext* _pCtx = a_context.pRenderContext;
		GraphicsEngine* _pGE = a_context.pGraphicsEngine;
		if (!_pCtx || !_pGE || !a_context.pCmdList || !a_context.pGraph) return;

		auto* _pCmdList = a_context.pCmdList;

		// レイワールド更新・シェーダーテーブル更新
		Engine::Raytracing::RayEngine::Instance().Commit(_pCmdList);
		const auto& _instanceVec = Raytracing::RayEngine::Instance().GetInstanceVec();
		if (_instanceVec.empty()) return;

		// 解像度はこのパイプラインのもの(カメラごとに違うことがある)
		const UINT _width = static_cast<UINT>(a_context.pGraph->GetViewportWidth());
		const UINT _height = a_context.pGraph->GetViewportHeight();

		m_shaderTable.CommitInstanceBindLess(_instanceVec, _pCtx, _width, _height);

		// ディスクリプタヒープセット
		_pCtx->BindCopyHeapAndSumplerBindLess();

		// パイプラインとルートシグネチャセット
		_pCmdList->SetPipelineState1(m_rayPSO.Get());
		_pCtx->SetComputeRootSignature(m_rayPSO.GetRootSigHandle());

		// カメラバインド
		_pCtx->ComputeBindRootCBV(0, _pGE->GetCameraData());

		// レイワールドバインド
		Raytracing::RayEngine::Instance().BindTLAS(_pCtx);

		// 出力のUAVをバインド
		const Slot* _pOut = FindOutputSlot(MakeSlotID("Shadow"));
		if (!_pOut) return;

		D3D12::GPUResource* _pOutRes = a_context.GetResource(*_pOut);
		if (!_pOutRes) return;
		_pCtx->BindUAVBindLess(2, _pOutRes->GetUAV());

		// GBufferIndex : バインドレスの添字をシェーダーへ渡す
		const Slot* _pDepth = FindInputSlot(MakeSlotID("Depth"));
		const Slot* _pNormal = FindInputSlot(MakeSlotID("Normal"));
		if (!_pDepth || !_pNormal) return;

		D3D12::GPUResource* _pDepthRes = a_context.GetResource(*_pDepth);
		D3D12::GPUResource* _pNormalRes = a_context.GetResource(*_pNormal);
		if (!_pDepthRes || !_pNormalRes) return;

		GBufferIndex _gbIdx = {};
		_gbIdx.depth = static_cast<int>(_pDepthRes->GetSRV().GetIndex());
		_gbIdx.normal = static_cast<int>(_pNormalRes->GetSRV().GetIndex());
		_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<GBufferIndex>(_pCmdList, 3, _gbIdx);

		// 主光源
		//
		// レイは平行光へ1本しか飛ばさないので、配列ではなく先頭の1つだけを受け取る。
		// 実体は LightManager が持っていて、詰め直しは GraphicsEngine::Execute() 側で済んでいる
		const auto _sunCB = _pGE->RefLightManager()->GetSunLightCB();
		_pCtx->BindCB()->BindAndAttachDataComputeRootCBV(_pCmdList, 4, _sunCB);

		// ディスパッチ
		const auto& _desc = m_shaderTable.GetDispatchDesc();
		_pCmdList->DispatchRays(&_desc);
	}

	EPassEditResult RaytracingShadowPass::EditUpdate()
	{
		ImGui::TextDisabled("主光源へレイを1本飛ばして遮蔽を求めます");
		if (!m_isReady) ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "PSO not ready");
		return EPassEditResult::None;
	}

	void RaytracingShadowPass::EditNode()
	{}

	void RaytracingShadowPass::Archive(Engine::Persistence::Archive& a_arch)
	{
		(void)a_arch;
	}
}
