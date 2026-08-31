#include "RaytracingGIPass.h"

#include "../../GraphicEngine.h"
#include "../../RenderContext/RenderContext.h"
#include "../../MeshBufferAllocator/MeshBufferAllocator.h"
#include "../RenderGraph/RenderGraph.h"

#include "../../../D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "../../../D3D12/PipelineStateManager/PipelineStateManager.h"
#include "../../../Raytracing/RaytracingEngine/RaytracingEngine.h"

namespace Engine::Graphics::Pipeline
{
	void RaytracingGIPass::SetupSlots()
	{
		// スロットは依存関係とバリアのために宣言する。
		// ルートパラメータ番号は付けない(バインドは自前でバインドレスに行うため)
		DeclareInput("Normal", EAccessType::SRV);
		DeclareInput("Depth", EAccessType::SRV);

		// GIはハーフ解像度で回る
		Slot& _out = DeclareOutput("GI", "RayGI", DXGI_FORMAT_R16G16B16A16_FLOAT,
			EAccessType::UAV);
		_out.loadOp = ELoadOp::Clear;
		_out.scale = 0.5f;
	}

	void RaytracingGIPass::Compile(const PassContext& a_context)
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
		_rayGlobal.AddDescriptorHeap({ {D3D12::RangeType::SRV,1} });	// インスタンス配列
		_rayGlobal.AddDescriptorHeap({ {D3D12::RangeType::SRV,2} });	// マテリアル
		_rayGlobal.AddRoot(D3D12::RootParameterType::RootCBV, 1);		// GBufferインデックス
		_rayGlobal.AddRoot(D3D12::RootParameterType::RootCBV, 10);		// 主光源
		_rayGlobal.AddDescriptorHeap({ {D3D12::RangeType::SRV,3} });	// 頂点
		_rayGlobal.AddDescriptorHeap({ {D3D12::RangeType::SRV,4} });	// インデックス
		_rayGlobal.AddDescriptorHeap({ {D3D12::RangeType::SRV,5} });	// アニメ済み頂点バッファ(t5)
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
		_psoInit.shaderPass = "Asset/Shader/Source/Raytracing/GI/RaytracingGI.hlsl";
		_psoInit.AddShader(L"RayGen", Raytracing::LocalRootSignature::RayGen, Raytracing::ShaderCategory::RayGenerator);
		_psoInit.AddShader(L"Miss", Raytracing::LocalRootSignature::Empty, Raytracing::ShaderCategory::Miss);
		_psoInit.AddShader(L"ClosestHit", Raytracing::LocalRootSignature::PBRMaterialHit, Raytracing::ShaderCategory::ClosestHit);
		_psoInit.AddShader(L"ShadowCHS", Raytracing::LocalRootSignature::PBRMaterialHit, Raytracing::ShaderCategory::ClosestHit);
		_psoInit.AddShader(L"ShadowMiss", Raytracing::LocalRootSignature::Empty, Raytracing::ShaderCategory::Miss);
		_psoInit.AddHitGroup(L"HitGroup", L"ClosestHit");
		_psoInit.AddHitGroup(L"ShadowHitGroup", L"ShadowCHS");

		// 影のレイを1本またぐので2段
		_psoInit.maxRecursionDepth = 2;

		_psoInit.globalRootSig = _pPSOManager->Request(_rayGlobal);
		_psoInit.hitRootSig    = _pPSOManager->Request(_hitSigInit);
		_psoInit.rayGenRootSig = _pPSOManager->Request(_rayGenSigInit);
		_psoInit.missRootSig   = _pPSOManager->Request(_missSigInit);

		if (!m_rayPSO.Init(_pDevice, _pPSOManager, _psoInit))
		{
			ENGINE_WARNING("[RaytracingGIPass] レイトレPSOの初期化に失敗しました");
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

	void RaytracingGIPass::Update(const PassContext& a_context)
	{
		if (!m_isReady) return;

		RenderContext* _pCtx = a_context.pRenderContext;
		GraphicsEngine* _pGE = a_context.pGraphicsEngine;
		if (!_pCtx || !_pGE || !a_context.pCmdList || !a_context.pGraph) return;

		auto* _pCmdList = a_context.pCmdList;

		auto* _pMA = _pGE->RefMeshBufferAllocator();
		if (!_pMA) return;

		// レイワールド更新・シェーダーテーブル更新
		Engine::Raytracing::RayEngine::Instance().Commit(_pCmdList);
		const auto& _instanceVec = Raytracing::RayEngine::Instance().GetInstanceVec();
		if (_instanceVec.empty()) return;

		// GIはハーフ解像度。出力リソースの実サイズから引く
		const Slot* _pOut = FindOutputSlot(MakeSlotID("GI"));
		if (!_pOut) return;

		const VirtualResource* _pOutVirtual = a_context.pGraph->GetVirtualResource(_pOut->resourceHandle);
		if (!_pOutVirtual) return;

		m_shaderTable.CommitInstanceBindLess(
			_instanceVec, _pCtx,
			static_cast<UINT>(_pOutVirtual->GetWidth()), _pOutVirtual->GetHeight());

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
		_gbIdx.frameCount = m_frameCount++;
		_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<GBufferIndex>(_pCmdList, 5, _gbIdx);

		// 主光源
		//
		// レイは平行光へ1本しか飛ばさないので、配列ではなく先頭の1つだけを受け取る。
		// 実体は LightManager が持っていて、詰め直しは GraphicsEngine::Execute() 側で済んでいる
		const auto _sunCB = _pGE->RefLightManager()->GetSunLightCB();
		_pCtx->BindCB()->BindAndAttachDataComputeRootCBV(_pCmdList, 6, _sunCB);

		// メッシュ情報バインド
		_pCtx->ComputeBindSRVBindLess(7, _pMA->GetStaticVertexBuffer().GetSRV());
		_pCtx->ComputeBindSRVBindLess(8, _pMA->GetIndexBuffer().GetSRV());

		// アニメ済み頂点バッファ(t5) : ヒットシェーダがスキニング済み頂点属性を読むために使う
		_pCtx->ComputeBindSRVBindLess(9, _pMA->GetAnimatedVertexBuffer().GetSRV());

		// ディスパッチ
		Raytracing::RayEngine::Instance().Dispatch(_pCtx, m_shaderTable);
	}

	EPassEditResult RaytracingGIPass::EditUpdate()
	{
		ImGui::TextDisabled("レイを飛ばして間接光を求めます(ハーフ解像度)");
		if (!m_isReady) ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "PSO not ready");
		return EPassEditResult::None;
	}

	void RaytracingGIPass::EditNode()
	{}

	void RaytracingGIPass::Archive(Engine::Persistence::Archive& a_arch)
	{
		(void)a_arch;
	}
}
