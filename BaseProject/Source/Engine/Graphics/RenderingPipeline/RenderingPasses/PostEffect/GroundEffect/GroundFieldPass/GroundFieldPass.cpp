#include "GroundFieldPass.h"

#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicsEngine.h"
#include "Engine/Graphics/Frame/RenderContext/RenderContext.h"

namespace Engine::Graphics::Pipeline
{
	void GroundFieldPass::SetupSlots()
	{
		// 地面だけの深度(GroundDepthPass の出力)
		DeclareInput("GroundDepth", EAccessType::SRV, EPassSlotType::Texture, true, kRootInputSRV);

		// 地面のワールド座標。xyz = 位置 / w = 地面があれば 1、無ければ 0。
		// 全画素を書き潰すのでクリアは不要
		DeclareOutput("WorldPos", "GroundWorldPos", DXGI_FORMAT_R32G32B32A32_FLOAT,
			EAccessType::UAV, EPassSlotType::Texture, false, kRootOutputUAV);
	}

	void GroundFieldPass::Compile(const PassContext& a_context)
	{
		SetupComputeShader(a_context, "Asset/Shader/Source/PostProcess/GroundEffect/GroundFieldCS.cso", "GroundFieldCS");
	}

	void GroundFieldPass::Update(const PassContext& a_context)
	{
		RenderContext* _pCtx = a_context.pRenderContext;
		GraphicsEngine* _pGE = a_context.pGraphicsEngine;
		if (!_pCtx || !_pGE || !a_context.pCmdList) return;

		auto* _pCmd = a_context.pCmdList;

		// カメラ : 深度からワールド座標を戻す
		_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<CameraData>(
			_pCmd, kRootCameraCB, _pGE->GetSceneView()->GetCameraData());

		// 経過時間と衝撃の数
		const float _deltaTime = MainEngine::Instance().GetDeltaTime();
		m_elapsedTime += _deltaTime;

		GroundFieldCB _cb = {};
		_cb.time = m_elapsedTime;
		_cb.deltaTime = _deltaTime;
		_cb.impulseCount = _pGE->GetGroundImpulseCount();
		_pCtx->BindCB()->BindAndAttachDataComputeRootCBV(_pCmd, kRootGroundFieldCB, _cb);

		// 衝撃の配列
		const UINT _impulseIndices[] = {
			_pGE->GetGroundImpulseBuffer().GetSRV().GetIndex(),
		};
		_pCtx->ComputeBindDescriptorIndices(kRootImpulseSRV, _impulseIndices);

		DispatchFullScreen(a_context);
	}



	void GroundFieldPass::Archive(Engine::Persistence::Archive& a_arch)
	{
		(void)a_arch;
	}
}
