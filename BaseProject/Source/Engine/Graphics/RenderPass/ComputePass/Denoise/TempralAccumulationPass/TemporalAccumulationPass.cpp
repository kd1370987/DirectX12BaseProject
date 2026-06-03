#include "TemporalAccumulationPass.h"

#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "../../../../GraphicEngine.h"
#include "../../../../RenderContext/RenderContext.h"

namespace Engine::Graphics
{
	void TemporalAccumulationPass::Excute(GraphicsEngine* a_pGE, RenderContext* a_pCtx)
	{
		// ルートシグネチャ、PSOをセット
		SetPSO(a_pCtx);

		// ヒープをセット
		a_pCtx->BindHeap();
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> _cpuVec = {};
		_cpuVec = {
			m_pRG->GetSRVCPU("RayGI"),
			m_pRG->GetSRVCPU("GBufferVelocity"),
			m_pRG->GetSRVCPU("DenoiseGI"),
			m_pRG->GetSRVCPU("Depth"),
			m_pRG->GetSRVCPU("GBufferNormal"),
			m_pRG->GetSRVCPU("PrevDepth"),
			m_pRG->GetSRVCPU("PrevNormal")
		};
		a_pCtx->ComputeBindSRV(0, _cpuVec);

		// UAVセット
		a_pCtx->BindUAV(1, m_pRG->GetUAVCPU("DenoiseGI"));

		a_pCtx->Dispatch(1280,720,1);

	}

	void TemporalAccumulationPass::CreatePass()
	{
		// パス設定
		SetName("TemporalAccumulationPass");

		SetShader("Asset/Shader/Compute/TemporalAccumulationShader/TemporalAccumulationShader.cso");

		// 依存関係
		AddRead("RayGI", AccessType::SRV, LoadOp::Load, StoreOp::Store);			// 現在フレームのGI
		AddRead("GBufferVelocity", AccessType::SRV, LoadOp::Load, StoreOp::Store);	// モーションベクター
		AddRead("DenoiseGI", AccessType::SRV, LoadOp::Load, StoreOp::Store);		// 前フレームのGI
		AddRead("Depth", AccessType::SRV, LoadOp::Load, StoreOp::Store);			// 現在深度
		AddRead("GBufferNormal", AccessType::SRV, LoadOp::Load, StoreOp::Store);	// 現在法線
		AddRead("PrevDepth", AccessType::SRV, LoadOp::Load, StoreOp::Store);		// 過去の深度
		AddRead("PrevNormal", AccessType::SRV, LoadOp::Load, StoreOp::Store);		// 過去の法線

		// 出力結果
		AddWrite("DenoiseGI", AccessType::UAV, LoadOp::Load, StoreOp::Store);
	}
}
