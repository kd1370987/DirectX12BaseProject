#include "GISpatialDenoisePass.h"
namespace Engine::Graphics
{
	void GISpatialDenoisePass::Excute(GraphicsEngine* a_pGE, RenderContext* a_pCtx)
	{
	}
	void GISpatialDenoisePass::CreatePass()
	{
		// パス設定
		SetName("GISpatialDenoisePass");

		SetShader("Asset/Shader/Compute/");

		// 依存関係
		AddRead("DenoiseGI", AccessType::SRV, LoadOp::Load, StoreOp::Store);		// デノイズされたGI
		AddRead("Depth", AccessType::SRV, LoadOp::Load, StoreOp::Store);			// 現在深度
		AddRead("GBufferNormal", AccessType::SRV, LoadOp::Load, StoreOp::Store);	// 現在法線
		

		// 出力結果
		AddWrite("FinalGI", AccessType::UAV, LoadOp::Load, StoreOp::Store);
	}
}