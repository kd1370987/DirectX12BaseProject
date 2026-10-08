#include "Engine/Graphics/RenderingPipeline/RenderingPasses/Lighting/Shadow/ShadowMapMaskPass/ShadowMapMaskPass.h"

#include "Engine/Graphics/GraphicsEngine.h"
#include "Engine/Graphics/Frame/RenderContext/RenderContext.h"
#include "Engine/Graphics/Frame/SceneView/CameraData.h"

namespace Engine::Graphics::Pipeline
{
	void ShadowMapMaskPass::SetupSlots()
	{
		// 描き足す先(レイトレの影)。
		// 中身は出力と同じものなので、シェーダーへは張らない(ルート番号を持たせない)
		DeclareInput("Shadow", EAccessType::UAV, EPassSlotType::Texture, false);

		// テーブルの並びはシェーダーの t0.. と同じ順にすること
		DeclareInput("Depth", EAccessType::SRV, EPassSlotType::Texture, true, ROOT_INPUT_SRV);
		DeclareInput("Normal", EAccessType::SRV, EPassSlotType::Texture, true, ROOT_INPUT_SRV);
		DeclareInput("ShadowMap", EAccessType::SRV, EPassSlotType::Texture, true, ROOT_INPUT_SRV);

		// レイトレの影と同じ形。レイトレの影が来ていれば、そのリソースへ書く(OnLinksResolved)
		Slot& _out = DeclareOutput("Shadow", "SunShadowMask", DXGI_FORMAT_R8G8B8A8_UNORM,
			EAccessType::UAV, EPassSlotType::Texture, false, ROOT_OUTPUT_UAV);
		_out.loadOp = ELoadOp::Load;
	}

	// レイトレの影が繋がっていれば、そのリソースへ書く
	void ShadowMapMaskPass::OnLinksResolved()
	{
		FollowInputToOutput("Shadow", "Shadow");
	}

	void ShadowMapMaskPass::Compile(const PassContext& a_context)
	{
		SetupComputeShader(a_context,
			"Asset/Shader/Source/Lighting/Shadow/ShadowMapMaskCS.cso",
			"ShadowMapMaskCS");
	}

	void ShadowMapMaskPass::Update(const PassContext& a_context)
	{
		RenderContext* _pCtx = a_context.pRenderContext;
		GraphicsEngine* _pGE = a_context.pGraphicsEngine;
		if (!_pCtx || !_pGE || !a_context.pCmdList) return;

		const LightManager* _pLightManager = _pGE->RefLightManager();

		// レイトレで求めるフレームは、レイトレの影をそのまま流す。
		// 単体で置かれている(描き足す先が無い)ときは自分しか書き手が居ないので、
		// シェーダーを回して「影なし」で埋める(カスケード 0 のときシェーダーがそうする)
		const Slot* _pBase = FindInputSlot(MakeSlotID("Shadow"));
		const bool _hasBase = (_pBase && _pBase->IsConnected());
		if (_hasBase && _pLightManager->GetShadowMode() != EDirectionalShadowMode::ShadowMap) return;

		// カメラ : 深度からワールド座標とビュー空間の奥行きを戻す
		_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<CameraData>(
			a_context.pCmdList, ROOT_CAMERA_CB, _pGE->GetSceneView()->GetCameraData());

		// カスケードの行列・区切り・バイアス。
		// ShadowMapPass が描いたときと同じもの(どちらも LightManager が組んだものを読む)
		_pCtx->BindCB()->BindAndAttachDataComputeRootCBV(
			a_context.pCmdList, ROOT_SHADOW_CB, _pLightManager->GetSunShadowCB());

		DispatchFullScreen(a_context);
	}



	void ShadowMapMaskPass::Archive(Engine::Persistence::Archive& a_arch)
	{
		(void)a_arch;
	}
}
