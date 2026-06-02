#include "TemporalAccumulationPass.h"

#include "../../../../GraphicEngine.h"
#include "../../../../RenderContext/RenderContext.h"

namespace Engine::Graphics
{
	void TemporalAccumulationPass::Excute(GraphicsEngine* a_pGE, RenderContext* a_pCtx)
	{
	}

	void TemporalAccumulationPass::CreatePass()
	{
		// パス設定
		SetName("TemporalAccumulationPass");

		SetShader("");

		// 依存関係
		AddRead("RayGI", AccessType::SRV, LoadOp::Load, StoreOp::Store);			// 現在フレームのGI
		AddRead("GBufferVelocity", AccessType::SRV, LoadOp::Load, StoreOp::Store);	// モーションベクター
		//AddRead("HistoryGI", AccessType::SRV, LoadOp::Load, StoreOp::Store);		// 前フレームのGI
		AddRead("Depth", AccessType::SRV, LoadOp::Load, StoreOp::Store);			// 現在深度
		AddRead("GBufferNormal", AccessType::SRV, LoadOp::Load, StoreOp::Store);	// 現在法線
		//AddRead("PrevDepth", AccessType::SRV, LoadOp::Load, StoreOp::Store);		// 過去の深度
		//AddRead("PrevNormal", AccessType::SRV, LoadOp::Load, StoreOp::Store);		// 過去の法線

		// 出力結果
		AddWrite("DenoiseGI", AccessType::UAV, LoadOp::Load, StoreOp::Store);
	}
}
